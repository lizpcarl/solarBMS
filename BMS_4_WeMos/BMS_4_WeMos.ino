/**
 * Author: Zheping.Li
 * Create Date:2023-03-13
 * Modify Date:2023-03-13
 * Target: 48v蓄电池组分4个独立控制子系统，每个子系统含：主蓄电池、备蓄电池、arduino、电压传感器、5v继电器(10A)、12v大功率继电器(30A)、连接线等。
 * Feature 1:
 * 1.使用了电压传感器 voltage sensor，每秒测试一次电压；
 * 2.默认初始状态是仅对主电池充电，为状态0；当主电池电压大于14.2v时，就启动对应的一个备用电池连接，对应的备用电池分流充电，进入状态1（充电分流模式）；
 * 3.状态1时，太阳能供电电压如果下降到12.1v以下，说明供电不足，立即切换到状态0；
 * 4.状态1时，电压在12.1~13.6之间，则启动延时计数，计到30次才切换回状态0；
 * 5.状态0时，电压低于11.8v进入状态2（放电补偿模式）；
 * 6.状态2时，电压>13.6v时，立即切换回状态0
 * 7.状态2时，电压在12.1~13.6之间，则启动延时计数，计到30次才切换回状态0；
 * 
 * Feature 2:
 * 连接wifi，通过UDP向局域网内某个linux主机上报实时电压以及状态。带wifi功能的Arduino，会主动连接一个SSID，向指定服务器的udp端口上报当前的电压
 * 
 * Notice:
 * 1.某款D1的wifi模块有点问题，连接速度特别慢，需要延长时间到1分钟左右才可以连接上，而且只能连接Tenda那个路由器(channel)，查找路由SSID时也特别慢，所以该程序调整为3秒一个循环。
 * 2.WeMos板子的问题，无法支持4路继电器，最后试验结果是使用D4、D2、D10、D14四个接口来控制，其中D4对应1号位继电器，其余的（D2、D10、D14）依赖D2接口的设备信号。
 *   2-->D4, 12-->D2，4-->D10,D14；
 * 3.Arduino上连接的中间继电器是4路低电平触发。
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
const char* myName = "BMS_4";
const int resetPerSecond = 1800;//对其它3块开发板重启的时间，每隔这么多秒就重启一次。

//ESP8266WiFiMulti WiFiMulti;
WiFiUDP Udp;

int output1Pin = 2;
int output2Pin = 12;
int output3Pin = 4;
int solarState = 0;//默认状态，对主电瓶正常充放电
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
  float voltage = sensorValue * (5.0 / 1023.0) * 3.19;
  // print out the value you read:
  Serial.printf("\r\nvoltage: %f,",voltage);

  switch(solarState){
    case 0:
        if(voltage > 14.2){
          solarState = 1;
          digitalWrite(output1Pin, LOW);
          lowVolCount = 0;
        }else if(voltage < 11.8){
          solarState = 2;
          digitalWrite(output1Pin, LOW);
          lowVolCount = 0;
        }
        break;
    case 1://备用电池分流
        if(voltage > 13.6){
          lowVolCount = 0;
        }else if(voltage < 12.1){//disconnect immediately
          solarState = 0;
          digitalWrite(output1Pin, HIGH);
          lowVolCount = 0;
        }else{//12.1~13.6
          int chargeSeconds = 0;
          chargeSeconds = (int)(30-(13.6-voltage)*20);
          if(lowVolCount++ > chargeSeconds){
            solarState = 0;
            digitalWrite(output1Pin, HIGH);
            lowVolCount = 0;
          }
        }
        break;
    case 2://备用电池补偿供电
        if(voltage < 12.1){
          lowVolCount = 0;
        }else if(voltage > 13.6){//disconnect immediately
          solarState = 0;
          digitalWrite(output1Pin, HIGH);
          lowVolCount = 0;
        }else{//12.1~13.6
          int chargeSeconds = 0;
          chargeSeconds = (int)((13.6-voltage)*20-0);
          if(lowVolCount++ > chargeSeconds){
            solarState = 0;
            digitalWrite(output1Pin, HIGH);
            lowVolCount = 0;
          }
        }
        break;
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
