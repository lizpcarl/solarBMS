/**
 * Author: Zheping.Li
 * Create Date:2023-03-13
 * Modify Date:2023-03-15
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
 * Notice:
 * 1.D1使用12的GPIO接口。
 * 3.编译时选择开发板为“Arduino AVR Boards-Arduino UNO”(Tools -> Board and select Arduino UNO)
 */

const int resetPerSecond = 60;//对第4块开发板重启，每隔这么多秒就重启一次。

int output1Pin = 5;
int output2Pin = 6;
int solarState = 0;//默认状态，对主电瓶正常充放电
int lowVolCount = 0;//低电压告警延时计数
int timeCount = 0;
int ledOnOff = 0;//板载led灯的亮灭控制参数。

// the setup routine runs once when you press reset:
void setup() {
    // initialize serial communication at 9600 bits per second:
    Serial.begin(115200);
    // pinMode(LED_BUILTIN, OUTPUT);//LED_BUILTIN equals 2;
    pinMode(output1Pin, OUTPUT);
    pinMode(output2Pin, INPUT);
    // digitalWrite(output1Pin, LOW);
    // digitalWrite(output2Pin, LOW);
    
    // delay(2000);
    // pinMode(output1Pin, INPUT);
    // delay(2000);
    // pinMode(output1Pin, OUTPUT);
    // delay(2000);
    // pinMode(output1Pin, INPUT);
}
void pin1Enable(){
    pinMode(output1Pin, OUTPUT);
    //digitalWrite(output1Pin, HIGH);
}
void pin1Disable(){ 
    pinMode(output1Pin, INPUT);
    //digitalWrite(output1Pin, LOW);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);Serial.println(sensorValue);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  // float voltage = sensorValue * (5.0 / 1023.0) * 3.21;
  float voltage = sensorValue * (13.1 / 563.0) ;
  // print out the value you read:
  // Serial.printf("\r\nvoltage: %f,",voltage);
  Serial.print(voltage);

  switch(solarState){
    case 0:
        if(voltage > 14.2){
          solarState = 1;
          pin1Enable();
          lowVolCount = 0;
        }else if(voltage < 11.8){
          solarState = 2;
          pin1Enable();
          lowVolCount = 0;
        }
        break;
    case 1://备用电池分流
        if(voltage > 13.6){
          lowVolCount = 0;
        }else if(voltage < 12.1){//disconnect immediately
          solarState = 0;
          pin1Disable();
          lowVolCount = 0;
        }else{//12.1~13.6
          int chargeSeconds = 0;
          chargeSeconds = (int)(30-(13.6-voltage)*20);
          if(lowVolCount++ > chargeSeconds){
            solarState = 0;
            pin1Disable();
            lowVolCount = 0;
          }
        }
        break;
    case 2://备用电池补偿供电
        if(voltage < 12.1){
          lowVolCount = 0;
        }else if(voltage > 13.6){//disconnect immediately
          solarState = 0;
          pin1Disable();
          lowVolCount = 0;
        }else{//12.1~13.6
          int chargeSeconds = 0;
          chargeSeconds = (int)((13.6-voltage)*20-0);
          if(lowVolCount++ > chargeSeconds){
            solarState = 0;
            pin1Disable();
            lowVolCount = 0;
          }
        }
        break;
  }
  // Serial.printf("ChargeState: %d\n",solarState);
  Serial.print(" ChargeState:");
  Serial.println(solarState);

  if(timeCount++ > resetPerSecond){
    pinMode(output2Pin, OUTPUT);//digitalWrite(output2Pin, HIGH);
    if(timeCount > resetPerSecond+3){
      pinMode(output2Pin, INPUT);//digitalWrite(output2Pin, LOW);
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
  // }

  delay(1000);
}
