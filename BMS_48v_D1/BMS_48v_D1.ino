/**
 * Author: Zheping.Li
 * Create Date:2023-10-4
 * Modify Date:2023-10-4
 * Target:
 * Feature 1:
 * 1.使用了电压传感器 voltage sensor，每秒测试一次电压；
 * 2.
 * 
 * Feature 2:
 * 连接wifi，通过UDP向局域网内某个linux主机上报实时电压以及状态。带wifi功能的Arduino，会主动连接一个SSID，向指定服务器的udp端口上报当前的电压
 * 
 * Notice:
 * 1.//Pin数字4对应硬件D4输出口，Pin数字12对应硬件D6输出口，Pin数字14对应硬件上的4、13这两个输出口，
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
const char* myName = "solar_48v";
const int resetPerSecond = 1800;//对其它3块开发板重启的时间，每隔这么多秒就重启一次。

//ESP8266WiFiMulti WiFiMulti;
WiFiUDP Udp;

int output1Pin = 4;
int output2Pin = 12;
int output3Pin = 14;
int solarState = 4;//默认状态，对4路导通充电
int lowVolCount = 0;//低电压告警延时计数
int highVolCount = 0;//高电压告警延时计数
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
    digitalWrite(output1Pin, HIGH);
    digitalWrite(output2Pin, HIGH);
    digitalWrite(output3Pin, HIGH);

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
//         digitalWrite(LED_BUILTIN, HIGH);
    }else{
//         digitalWrite(LED_BUILTIN, LOW);
        ledOnOff = 1;
    }

}
void switchByState(int newState){
  switch(newState){
      case 0:
        digitalWrite(output1Pin, LOW);
        digitalWrite(output2Pin, LOW);
        digitalWrite(output3Pin, LOW);
        break;
      case 1:
          digitalWrite(output1Pin, HIGH);
          digitalWrite(output2Pin, LOW);
          digitalWrite(output3Pin, LOW);
        break;
      case 2:
          digitalWrite(output1Pin, LOW);
          digitalWrite(output2Pin, HIGH);
          digitalWrite(output3Pin, HIGH);
        break;
      case 4:
          digitalWrite(output1Pin, HIGH);
          digitalWrite(output2Pin, HIGH);
          digitalWrite(output3Pin, HIGH);
        break;
  }
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0) * 13.99;//采用1k/4k的电阻分压48v电压，最高可能达到90v，分压最高可能达到23v。
  // print out the value you read:
  Serial.printf("\r\nvoltage: %f,",voltage);

    if(voltage > 59.0){//立减全部
        solarState = 0;
        switchByState(solarState);
        lowVolCount = 0;
        highVolCount = 0;
    }else if(voltage > 56.0){//56~59之间，准备减1路
        if(highVolCount > 30-10*(int)(voltage-56)){//连续30秒高电压，
            if(solarState == 4){
                solarState = 2;
            }else if(solarState == 2){
                solarState = 1;
            }else if(solarState == 1){
                solarState = 0;
            }
            switchByState(solarState);
            highVolCount = 0;
        }else{
            highVolCount++;
        }
        lowVolCount = 0;
    }else if(voltage > 54.0){//54~56v之间，不处理
        lowVolCount = 0;
        highVolCount = 0;
    }else if(voltage > 51.0){//51~54之间，准备增加1路
        if(lowVolCount > 30-10*(54-voltage)){//连续30秒低电压，
            if(solarState == 0){
                solarState = 1;
            }else if(solarState == 1){
                solarState = 2;
            }else if(solarState == 2){
                solarState = 4;
            }
            switchByState(solarState);
            lowVolCount = 0;
        }else{
            lowVolCount++;
        }
        highVolCount = 0;
    }else{//小于51v，电压过低立即全加上
        solarState = 4;
        switchByState(solarState);
        lowVolCount = 0;
        highVolCount = 0;
    }
    Serial.printf("ChargeState: %d\n",solarState);

  if(timeCount++ > resetPerSecond){
    digitalWrite(output2Pin, LOW);
    digitalWrite(output3Pin, LOW);
    if(timeCount > resetPerSecond+3){
      digitalWrite(output2Pin, HIGH);
      digitalWrite(output3Pin, HIGH);
      timeCount = 0;
    }
  }

//以下这段为测试低电平触发器的工作状态
//   if(timeCount % 5 == 0){
//     digitalWrite(output1Pin, LOW);
//   }else if((timeCount-1) % 5 == 0){
//     digitalWrite(output1Pin, HIGH);
//     digitalWrite(output2Pin, LOW);
//   }else if((timeCount-2) % 5 == 0){
//     digitalWrite(output2Pin, HIGH);
//     digitalWrite(output3Pin, LOW);
//   }else if((timeCount-3) % 5 == 0){
//     digitalWrite(output3Pin, HIGH);
//   }

  if(timeCount % 3 == 0 && wifiState == 2 && WiFi.status() == WL_CONNECTED){
    //  WiFiClient client;client.connect(host, port);client.println("");
    if (!Udp.beginPacket(host, port)) {
      Serial.println("connection failed");
    }else{
      String infoJson = "{name:\""+ String(myName) +"\",voltage:"+ String(voltage) +", state:"+solarState+"}\r\n";
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
