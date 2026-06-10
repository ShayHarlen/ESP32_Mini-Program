#include <Arduino.h>
#define key 5
bool key_status = false;
unsigned long presstime;

void key_check_run(){
  if(key_status==false)
  {
    if (digitalRead(key)==LOW)
    {
      delay(10);
      if (digitalRead(key)==LOW)
      {
        key_status=true;
      }
      presstime=millis();
    }
  }
  if(key_status==true){
    if (digitalRead(key)==HIGH){
      if (millis()-presstime>1000)
      {
         Serial.printf("LONG\n");
        
      }
      else
      {
        Serial.printf("short\n");
      }
       key_status=false;
    }

  }
  
}

void setup() {
  // put your setup code here, to run once:
 Serial.begin(115200);
 Serial.printf("setup\n");
pinMode(18,INPUT);
}

void loop() {
  delay(10);
  key_check_run();
}
