#include <Arduino.h>
#define PIN_LED 18

 void LEDFLASH(int ms)
 {
    digitalWrite(18,LOW);
    delay(ms);
    digitalWrite(18,HIGH);
    delay(ms);
 }
void led_status(int status)
{
    if(status==0)
    {
    digitalWrite(18,LOW);
    }
    else if (status==1)
    {
    digitalWrite(18,HIGH);
    }
    else if (status==2)
    {
   LEDFLASH(250);
    }
    else if (status==3)
    {
   LEDFLASH(1000);
    }
}

void setup() {
 Serial.begin(115200);
 Serial.printf("setup\n");

 pinMode(18,OUTPUT);

}

void loop() {
    /*串口打印
 Serial.printf("hello world\n");
 delay(1000);*/
led_status(3);
}

