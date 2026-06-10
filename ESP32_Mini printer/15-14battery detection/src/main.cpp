#include <Arduino.h>
#define PIN_BATTERY_ADC 34
int readvoltage(){
  int data=0;
  data = analogReadMilliVolts(PIN_BATTERY_ADC);
  return data;
}
/*电池满电4.2 有电3.3 但电路是分压电路 读取的是一般的电压 所以要乘2 同时readvoltage的单位是mv */
void Batterylevel(){
   unsigned char level=map(readvoltage()*2,3300,4200,0,100);
   Serial.printf("电量=%d\n",level);
}

void setup() {
 Serial.begin(115200);
 Serial.printf("setup\n");
}

void loop() {
   delay(1000);
  Batterylevel();
}

