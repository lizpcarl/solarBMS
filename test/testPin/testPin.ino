int pin1 = 3;//D5,D6,D12,D13均可正常，但digitalWrite low的电压有1.5v，部分继电器无法松开，只能换用pinMode来处理。


int output1Pin = 2;
int output2Pin = 12;
int output3Pin = 4;

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(pin1, OUTPUT);
  pinMode(output1Pin, OUTPUT);
}

void loop() {
  pinMode(pin1, INPUT);//digitalWrite(pin1, HIGH);  // turn the LED on (HIGH is the voltage level)

  // digitalWrite(output1Pin, HIGH);
  digitalWrite(output2Pin, HIGH);
  digitalWrite(output3Pin, HIGH);
  
  delay(1000);                      // wait for a second
  pinMode(pin1, OUTPUT);//digitalWrite(pin1, LOW);   // turn the LED off by making the voltage LOW
  
  // digitalWrite(output1Pin, LOW);
  digitalWrite(output2Pin, LOW);
  digitalWrite(output3Pin, LOW);
  delay(1000);                      // wait for a second
}