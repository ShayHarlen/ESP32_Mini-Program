#include <Arduino.h>

//打印头电源升压控制引脚
#define PIN_VHEN 17
#define PIN_PAPER 35
void read_paper_status(){
if (digitalRead(PIN_PAPER)==LOW)
{
  Serial.print("paper\n");
}
else{
   Serial.print("no paper\n");
}


}
void setup() {
  Serial.begin(115200);
  Serial.print("init_task\n");
  /*关闭打印加热头*/
  pinMode(PIN_VHEN, OUTPUT);
  digitalWrite(PIN_VHEN, LOW);
  /*缺纸检测IO*/
  pinMode(PIN_PAPER,INPUT);
}

void loop() {
  read_paper_status();
  delay(1000);
}