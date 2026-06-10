#include <Arduino.h>
#include <Ticker.h>

//电机引脚
#define PIN_MOTOR_AP 23
#define PIN_MOTOR_AM 22
#define PIN_MOTOR_BP 21
#define PIN_MOTOR_BM 19

//打印头电源升压控制引脚
#define PIN_VHEN 17

//电机运行一步的时间 2ms/500Hz 
#define MOTOR_WATI_TIME 2   
// ================= 🔴 核心变量区 =================
// 【关键！】必须加 volatile，告诉系统这俩变量是在中断里跑的
volatile uint8_t location = 0;       
volatile int remaining_steps = 0; // 老板写任务的黑板！

uint8_t motor_table[8][4] =
    {
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {1, 0, 1, 0},
        {1, 0, 0, 0},
        {1, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 1, 0, 1},
        {0, 1, 0, 0}};



void init_motor()
{
    pinMode(PIN_MOTOR_AP, OUTPUT);
    pinMode(PIN_MOTOR_AM, OUTPUT);
    pinMode(PIN_MOTOR_BP, OUTPUT);
    pinMode(PIN_MOTOR_BM, OUTPUT);

    digitalWrite(PIN_MOTOR_AP, 0);
    digitalWrite(PIN_MOTOR_AM, 0);
    digitalWrite(PIN_MOTOR_BP, 0);
    digitalWrite(PIN_MOTOR_BM, 0);
}

void IRAM_ATTR move_motor(){
  if (remaining_steps>0)
  {
  digitalWrite(PIN_MOTOR_AP, motor_table[location][0] );
  digitalWrite(PIN_MOTOR_AM, motor_table[location][1] );
  digitalWrite(PIN_MOTOR_BP, motor_table[location][2] );
  digitalWrite(PIN_MOTOR_BM, motor_table[location][3] );
  location++;
  if(location>=8)
  {
    location=0;
  }
  remaining_steps--;
        if (remaining_steps == 0) {
            digitalWrite(PIN_MOTOR_AP, 0);
            digitalWrite(PIN_MOTOR_AM, 0);
            digitalWrite(PIN_MOTOR_BP, 0);
            digitalWrite(PIN_MOTOR_BM, 0);
        }
  }


  
  
}

void set_motor(int step){
  remaining_steps=step;
}

// ================= 🟢 库直接调用区 =================

// 1. 声明一个硬件定时器对象（直接抄）
hw_timer_t * timer_motor = NULL; 

void init_timer() {
  // 2. 初始化定时器0，80分频（代表1微秒跳1次），向上计数
  timer_motor = timerBegin(0, 80, true);
  
  // 3. 绑定你的专属中断函数（&onTimer_ISR 是你等下要写的函数名）
  timerAttachInterrupt(timer_motor, &move_motor, true);
  
  // 4. 设置闹钟时间：2000 微秒 (即 2ms)，true代表一直循环响
  timerAlarmWrite(timer_motor, 2000, true);
  
  // 5. 开启闹钟
  timerAlarmEnable(timer_motor);
}
// ===================================================

void setup() {
  Serial.begin(115200);
  Serial.printf("setup\n");
  pinMode(PIN_VHEN, OUTPUT);
  digitalWrite(PIN_VHEN, LOW);
  init_motor();
  init_timer();
}

void loop() {
  set_motor(5);

}
