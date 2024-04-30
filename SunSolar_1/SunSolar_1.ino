/**
 * Author: Zheping.Li
 * Create Date:2024-04-30
 * Modify Date:2024-04-30
 * Target: 孙12v监控系统停电保障，太阳能与市电交替给监控系统供电。
 * Feature 1:
 * 1.使用了电压传感器 voltage sensor，每秒测试一次电压；
 * 2.默认初始状态是使用市电供电，为状态0；此时分2种情况：
      (1)市电12v有电时会通过switch3切断蓄电池的供电；
      (2)市电停电情况下，switch3的切断操作失效，会自动使用蓄电池供电；
      4路switch均为低电平；当主电池电压大于13.5v时，就强行切换到太阳能蓄电池供电，进入状态1(switch1高电平，使市电12v断开；switch3高电平，蓄电池供电导通)；
 * 3.状态1时，太阳能供电电压如果下降到11.8v以下，说明供电不足，立即切换到状态0；
 * 4.状态1时，电压在12.0~12.5之间，则启动延时计数，计到30次才切换回状态0；
 * 5.不论是状态0还是1，都检测蓄电池电压，如果电压大于13.2v时，就把switch2改为高电平，启动降压电路。
 * 6.状态0时，定时重启Arduino，静默重置控制系统，switch4高电平一次；
 * 
 * Feature 2:
 * 连接wifi，通过UDP向局域网内某个linux主机上报实时电压以及状态。带wifi功能的Arduino，会主动连接一个SSID，向指定服务器的udp端口上报当前的电压
 * 
 * Notice:
 * 1.D0对应第1口，D15对应第4口，D14对应第5口，D12对应第7口
 * 3.Arduino上连接的中间继电器是4路高电平触发。
 */
#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "mySSID"
#define STAPSK  "12345678"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "172.18.8.97";
const uint16_t port = 8048;
const char* myName = "SunSolar_1";
const int resetPerSecond = 1800;//对开发板重启的时间，每隔这么多秒就重启一次。

//ESP8266WiFiMulti WiFiMulti;
WiFiUDP Udp;

int output1Pin = D0;
int output2Pin = D15;
int output3Pin = D14;
int output4Pin = D12;
int solarState = 0;//默认状态，对主电瓶正常充放电
int ConvertVoltage2Low = 0;
int lowVolCount = 0;//低电压告警延时计数
int timeCount = 0;
int wifiState = 0;//0未连接，1正在连接，2已连接成功
int wifiCount = 0;//尝试连接wifi的次数
int ledOnOff = 0;//板载led灯的亮灭控制参数。

// the setup routine runs once when you press reset:
void setup() {
    // initialize serial communication at 9600 bits per second:
    Serial.begin(115200);
    // pinMode(LED_BUILTIN, OUTPUT);//LED_BUILTIN equals 2;
    pinMode(output1Pin, OUTPUT);
    pinMode(output2Pin, OUTPUT);
    pinMode(output3Pin, OUTPUT);
    pinMode(output4Pin, OUTPUT);
    digitalWrite(output1Pin, LOW);
    digitalWrite(output2Pin, LOW);
    digitalWrite(output3Pin, LOW);
    digitalWrite(output4Pin, LOW);

    WiFi.mode(WIFI_STA);
}
void connectWifi(){
    if(wifiState == 0){
        // We start by connecting to a WiFi network
        WiFi.begin(STASSID, STAPSK);//WiFiMulti.addAP(ssid, password);
        wifiState = 1;
        wifiCount = 0;
        Serial.println();
        Serial.printf("正在连接Wifi:%s…",STASSID);
        ledOnOff = 2;
    }else if(wifiState == 1){
        if(WiFi.status() == WL_CONNECTED){//连接成功
            Serial.printf("连接成功，建立连接耗时%d秒\r\n", wifiCount);
            ledOnOff = 3;
            wifiState = 2;
            wifiCount = 0;
        }else{
            if(wifiCount++ > 156){
                Serial.printf("Wifi连接失败！%d\r 尝试%d秒。\r\n", WiFi.status(), wifiCount);
                ledOnOff = 2;
                wifiState = 0;
                WiFi.disconnect();
                wifiCount = 0;
            }else{
                Serial.print("connecting... ");
            }
        }
    }else{// ==2
        if(WiFi.status() != WL_CONNECTED){
            Serial.print("Wifi连接断开！稍后自动重试");
            ledOnOff = 3;
            wifiState = 0;
            WiFi.disconnect();
            wifiCount = 0;
        }
    }

    if(ledOnOff-- > 0){
        // digitalWrite(LED_BUILTIN, HIGH);
    }else{
        // digitalWrite(LED_BUILTIN, LOW);
        ledOnOff = 1;
    }

}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0) * 3.08;
  // print out the value you read:
  Serial.printf("\r\nvoltage: %f,",voltage);

  if(ConvertVoltage2Low > 0){
      if(voltage < 12.9){
          digitalWrite(output2Pin, LOW);
          ConvertVoltage2Low = 0;
      }
  }else{
      if(voltage > 13.2){
          digitalWrite(output2Pin, HIGH);
          ConvertVoltage2Low = 1;
      }
  }

  switch(solarState){
    case 0://市电12v供电情况
        if(voltage > 13.5){
          solarState = 1;
          digitalWrite(output1Pin, HIGH);
          digitalWrite(output3Pin, HIGH);
          lowVolCount = 0;
        }
        break;
    case 1://蓄电池供电情况
        if(voltage > 12.6){
          lowVolCount = 0;
        }else if(voltage < 11.8){//disconnect immediately
          solarState = 0;
          digitalWrite(output1Pin, LOW);
          digitalWrite(output3Pin, LOW);
          lowVolCount = 0;
        }else{//11.8~12.6
          int chargeSeconds = 0;
          chargeSeconds = (int)(32-(12.6-voltage)*40);
          if(lowVolCount++ > chargeSeconds){
            solarState = 0;
            digitalWrite(output1Pin, LOW);
            digitalWrite(output3Pin, LOW);
            lowVolCount = 0;
          }
        }
        break;
  }
  Serial.printf("ChargeState: %d, ConvertVoltage2Low:%d\n",solarState, ConvertVoltage2Low);

  if(solarState == 0 && timeCount++ > resetPerSecond){
    digitalWrite(output4Pin, HIGH);
    if(timeCount > resetPerSecond+3){
      digitalWrite(output4Pin, LOW);
      timeCount = 0;
    }
  }

//以下这段为测试低电平触发器的工作状态
  // if(timeCount % 5 == 0){
  //   digitalWrite(output1Pin, LOW);
  // }else if((timeCount-1) % 5 == 0){
  //   digitalWrite(output1Pin, HIGH);
  //   digitalWrite(output2Pin, LOW);
  // }else if((timeCount-2) % 5 == 0){
  //   digitalWrite(output2Pin, HIGH);
  //   digitalWrite(output3Pin, LOW);
  // }else if((timeCount-3) % 5 == 0){
  //   digitalWrite(output3Pin, HIGH);
  // }

  if(timeCount % 3 == 0 && wifiState == 2 && WiFi.status() == WL_CONNECTED){
    //  WiFiClient client;client.connect(host, port);client.println("");
    if (!Udp.beginPacket(host, port)) {
      Serial.println("connection failed");
    }else{
      String infoJson = "{name:\""+ String(myName) +"\",voltage:"+ String(voltage) +", state:"+solarState+", ConvertVoltage2Low:"+ConvertVoltage2Low+"}\r\n";
      char  sendPacket[] = {"\0"};
      strcpy(sendPacket,infoJson.c_str());
      Udp.write(sendPacket);
      Udp.endPacket();
      //read back one line from server
    }

    //String line = client.readStringUntil('\r');
//    char incomingPacket[256];
//    int len = Udp.read(incomingPacket, 255);
//    if(len > 0){
//      incomingPacket[len] = '\0';
//      Serial.println("receiving from remote server");
//      Serial.printf("UDP packet contents: %s\n", incomingPacket);
//    }
    }

    delay(1000);
    connectWifi();
}
