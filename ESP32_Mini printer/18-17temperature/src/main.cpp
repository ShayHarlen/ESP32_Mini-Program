#include <Arduino.h>

//打印头电源升压控制引脚
#define PIN_VHEN 17
//温度检测引脚
#define PIN_temperature 36

float voltage;
float resistance;
float temperature;
float Calculate_temperature(){
  voltage=analogReadMilliVolts(PIN_temperature);
  resistance=(voltage*10)/(3.3-voltage/1000); //3.3/voltage=(10000+resistance)/resistance
  temperature=(1/(log(resistance/30000)/3950+1/298.15))-273.15;
  return temperature;
}

//怎么通过电阻（R_t）算出温度？
/*
R_t：当前温度下的真实阻值（第一步算出来的）。
R_p：标称阻值（常温 25℃ 时的阻值）。
T_2：常温 25℃ 的绝对温度（273.15 + 25 = 298.15K）。
B：热敏电阻的材料特性常数（代码里是 3950）。
T_1：我们想求的当前绝对温度！
当你看到 EXP[...] 时，它在数学上的严格书写形式就是 e^{[...]}。
公式如下 R_t=R_p*EXP[B*( 1/T_1 - 1/T_2 )]
所以 T_1 =1/((ln(R_t/R_p)/B)+1/T_2)
最后 因为都是用卡尔文温度在计算  所以要减去273.15
所以 T_1 =1/((ln(R_t/R_p)/B)+1/T_2)-273.15
*/

void setup() {
  Serial.begin(115200);
  Serial.print("init_task\n");
  /*关闭打印加热头*/
  pinMode(PIN_VHEN, OUTPUT);
  digitalWrite(PIN_VHEN, LOW);


}

void loop() {
  delay(1000);
  Serial.printf("温度=%f\n",Calculate_temperature());
}