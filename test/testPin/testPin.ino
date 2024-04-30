/**
 * 2.WeMos板子的问题，无法支持4路继电器，最后试验结果是使用D4、D2、D10、D14四个接口来控制，其中D4对应1号位继电器，其余的（D2、D10、D14）依赖D2接口的设备信号。
 *   2-->D4, 12-->D2，4-->D10,D14；
 */
int pin1 = 3;//D5,D6,D12,D13均可正常，但digitalWrite low的电压有1.5v，部分继电器无法松开，只能换用pinMode来处理。
//Pin数字4对应硬件D4输出口，Pin数字12对应硬件D6输出口，Pin数字14对应硬件上的4、13这两个输出口，
//D0对应第1口，D15对应第4口，D14对应第5口，D12对应第7口

int output1Pin = D0;
int output2Pin = D15;
int output3Pin = D14;
int output4Pin = D12;
int count = 0;
int arr[4] = {output1Pin,output2Pin,output3Pin,output4Pin};

void setup() {
    Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  // pinMode(pin1, OUTPUT);
  pinMode(output1Pin, OUTPUT);
  pinMode(output2Pin, OUTPUT);
  pinMode(output3Pin, OUTPUT);
  pinMode(output4Pin, OUTPUT);
    // digitalWrite(output1Pin, LOW);
    // digitalWrite(output2Pin, HIGH);
    // digitalWrite(output3Pin, HIGH);
}

void loop() {
  // pinMode(pin1, INPUT);//digitalWrite(pin1, HIGH);  // turn the LED on (HIGH is the voltage level)

  // digitalWrite(output1Pin, HIGH);
  // digitalWrite(output2Pin, HIGH);
  // digitalWrite(output3Pin, HIGH);
  // digitalWrite(output4Pin, HIGH);
  // Serial.println("HIGH");
  
  // delay(1000);                      // wait for a second
  // pinMode(pin1, OUTPUT);//digitalWrite(pin1, LOW);   // turn the LED off by making the voltage LOW
  
  // digitalWrite(output1Pin, LOW);
  // digitalWrite(output2Pin, LOW);
  // digitalWrite(output3Pin, LOW);
  // digitalWrite(output4Pin, LOW);
  // Serial.println("LOW");

  digitalWrite(arr[count % 4], HIGH);
  Serial.println(count % 4);
  Serial.println("LOW");
  delay(1000);  
  digitalWrite(arr[count % 4], LOW);
  count++;
  delay(1000);                      // wait for a second
}
