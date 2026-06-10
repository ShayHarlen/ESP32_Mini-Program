#include <Arduino.h>
#include <Ticker.h>

//////////////////////////////////////////////////电机部分//////////////////////////////////////////////////
#define MOTOR_CODE
#ifdef MOTOR_CODE
//电机引脚
#define PIN_MOTOR_AP 23
#define PIN_MOTOR_AM 22
#define PIN_MOTOR_BP 21
#define PIN_MOTOR_BM 19

#define MOTOR_WATI_TIME 2    //电机运行一步的时间 2ms/500Hz 

uint8_t motor_table[8][4] =
    {
        {1, 0, 0, 1},
        {0, 0, 0, 1},
        {0, 1, 0, 1},
        {0, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 0},
        {1, 0, 1, 0},
        {1, 0, 0, 0}
     };

uint8_t motor_pos = 0;

Ticker timer_motor;

void timer_motor_callbackfun()
{
    digitalWrite(PIN_MOTOR_AP, motor_table[motor_pos][0]);
    digitalWrite(PIN_MOTOR_AM, motor_table[motor_pos][1]);
    digitalWrite(PIN_MOTOR_BP, motor_table[motor_pos][2]);
    digitalWrite(PIN_MOTOR_BM, motor_table[motor_pos][3]);
    motor_pos++;
    if (motor_pos >= 8)
    {
        motor_pos = 0;
    }
}

void motor_start()
{
    if (timer_motor.active() == false)
        timer_motor.attach_ms(2, timer_motor_callbackfun);
}

void motor_stop()
{
    digitalWrite(PIN_MOTOR_AP, 0);
    digitalWrite(PIN_MOTOR_AM, 0);
    digitalWrite(PIN_MOTOR_BP, 0);
    digitalWrite(PIN_MOTOR_BM, 0);
    if (timer_motor.active())
        timer_motor.detach();
}

void motor_run()
{
    digitalWrite(PIN_MOTOR_AP, motor_table[motor_pos][0]);
    digitalWrite(PIN_MOTOR_AM, motor_table[motor_pos][1]);
    digitalWrite(PIN_MOTOR_BP, motor_table[motor_pos][2]);
    digitalWrite(PIN_MOTOR_BM, motor_table[motor_pos][3]);
    motor_pos++;
    if (motor_pos >= 8)
    {
        motor_pos = 0;
    }
}

void motor_run_step(uint32_t steps)
{
    while (steps)
    {
        digitalWrite(PIN_MOTOR_AP, motor_table[motor_pos][0]);
        digitalWrite(PIN_MOTOR_AM, motor_table[motor_pos][1]);
        digitalWrite(PIN_MOTOR_BP, motor_table[motor_pos][2]);
        digitalWrite(PIN_MOTOR_BM, motor_table[motor_pos][3]);
        motor_pos++;
        if (motor_pos >= 8)
        {
            motor_pos = 0;
        }
        delay(MOTOR_WATI_TIME);
        steps--;
    }
}

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
#endif
//////////////////////////////////////////////////SPI部分//////////////////////////////////////////////////
//打印的内容是通过SPI接口发送给打印头的，这里用的SPI有两根线，一根时钟，一根数据线
//一行有384个点，用384bit来表示，bit为0时不加热，bit为1时加热
//一个byte有8个bit，那一行就是发送384/8=48Byte的
#define SPI_CODE
#ifdef SPI_CODE
#include <SPI.h>

//打印头数据引脚
#define PIN_SCK 15
#define PIN_SDA 13
static const int spiClk = 1000000; // 1 MHz

SPIClass hspi  = SPIClass(HSPI);
SPISettings printerSPISettings = SPISettings(1000000, SPI_MSBFIRST, SPI_MODE0);

void spiCommand(uint8_t *data_buffer, uint8_t data_len)
{
    // use it as you would the regular arduino SPI API
    hspi.beginTransaction(printerSPISettings);
    hspi.transfer(data_buffer, data_len);
    hspi.endTransaction();
}

void init_spi()
{
    // hspi = SPIClass(HSPI);
    // alternatively route through GPIO pins
    //hspi.begin(PIN_SCK, -1, PIN_SDA, -1); // 课程原来的代码与新打印头适配时，初始化为-1时，会导致LAT锁存引脚不受控
    hspi.begin(PIN_SCK, 16, PIN_SDA, -1); // SCLK, MISO, MOSI, SS
    hspi.setFrequency(2000000);
}
#endif

//////////////////////////////////////////////////打印部分//////////////////////////////////////////////////
#define PRINTER_CODE
#ifdef PRINTER_CODE
//打印头数据锁存引脚
#define PIN_LAT 12
//通道引脚
//原厂 V3
#define PIN_STB1 26  
#define PIN_STB2 27  
#define PIN_STB3 14  
#define PIN_STB4 32  
#define PIN_STB5 33  
#define PIN_STB6 25 

//拆机 V2
// #define PIN_STB1 14
// #define PIN_STB2 27
// #define PIN_STB3 26
// #define PIN_STB4 25
// #define PIN_STB5 33
// #define PIN_STB6 32


#define PRINT_TIME 1700         //打印加热时间
#define PRINT_END_TIME 200      //冷却时间
#define LAT_TIME 1              //数据锁存时间
#define PIN_VHEN 17             //打印头电源升压控制引脚

/**
 * @brief 失能所有通道
 *
 */
static void set_stb_idle()
{
    digitalWrite(PIN_STB1, LOW);
    digitalWrite(PIN_STB2, LOW);
    digitalWrite(PIN_STB3, LOW);
    digitalWrite(PIN_STB4, LOW);
    digitalWrite(PIN_STB5, LOW);
    digitalWrite(PIN_STB6, LOW);
}

/**
 * @brief 打印前初始化
 *
 */
static void init_printing()
{
    set_stb_idle();
    digitalWrite(PIN_LAT, HIGH);
    // POWER ON
    digitalWrite(PIN_VHEN, HIGH);
}

/**
 * @brief 打印后停止
 *
 */
static void stop_printing()
{
    // POWER OFF
    digitalWrite(PIN_VHEN, LOW);
    set_stb_idle();
    digitalWrite(PIN_LAT, HIGH);
}

/**
 * @brief 发送一行数据
 *
 * @param data
 */
static void send_one_line_data(uint8_t *data)
{
    spiCommand(data, 48);
    /* After send one dot line, send LAT signal low pulse.*/
    digitalWrite(PIN_LAT, LOW);
    delayMicroseconds(LAT_TIME);
    digitalWrite(PIN_LAT, HIGH);
}

/**
 * @brief 通道打印运行
 *
 * @param now_stb_num
 */
static void run_stb(uint8_t now_stb_num)
{
    switch (now_stb_num)
    {
    case 0:
        digitalWrite(PIN_STB1, 1);
        delayMicroseconds(PRINT_TIME);
        digitalWrite(PIN_STB1, 0);
        delayMicroseconds(PRINT_END_TIME);
        break;
    case 1:
        digitalWrite(PIN_STB2, 1);
        delayMicroseconds(PRINT_TIME);
        digitalWrite(PIN_STB2, 0);
        delayMicroseconds(PRINT_END_TIME);
        break;
    case 2:
        digitalWrite(PIN_STB3, 1);
        delayMicroseconds(PRINT_TIME);
        digitalWrite(PIN_STB3, 0);
        delayMicroseconds(PRINT_END_TIME);
        break;
    case 3:
        digitalWrite(PIN_STB4, 1);
        delayMicroseconds(PRINT_TIME);
        digitalWrite(PIN_STB4, 0);
        delayMicroseconds(PRINT_END_TIME);
        break;
    case 4:
        digitalWrite(PIN_STB5, 1);
        delayMicroseconds(PRINT_TIME);
        digitalWrite(PIN_STB5, 0);
        delayMicroseconds(PRINT_END_TIME);
        break;
    case 5:
        digitalWrite(PIN_STB6, 1);
        delayMicroseconds(PRINT_TIME);
        digitalWrite(PIN_STB6, 0);
        delayMicroseconds(PRINT_END_TIME);
        break;
    default:
        break;
    }
}

/**
 * @brief 移动电机&开始打印
 *
 * @param need_stop
 * @param stbnum
 */
bool move_and_start_std(bool need_stop, uint8_t stbnum)
{
    if (need_stop == true)
    {
        motor_stop();
        stop_printing();
        return true;
    }
    // 4step一行
    motor_run();
    // 单通道打印
    run_stb(stbnum);
    motor_run_step(3);
    return false;
}

/**
 * @brief 单通道数组打印
 *
 * @param stbnum
 * @param data
 * @param len
 */
void start_printing_by_onestb(uint8_t stbnum, uint8_t *data, uint32_t len)
{
    uint32_t offset = 0;
    uint8_t *ptr = data;
    bool need_stop = false;
    //LAT设置为高 VH电源设置为高
    init_printing();
    while (1)
    {
        Serial.printf("printer %d\n", offset);
        if (len > offset)
        {
            // 发送一行数据 48byte*8=384bit
            send_one_line_data(ptr);
            // 每次偏移48Byte,直至所有数据发送完成
            offset += 48;
            ptr += 48;
        }
        else
            need_stop = true;
        if (move_and_start_std(need_stop, stbnum))
            break;
        //if(printing_error_check(false))
        //    break;
    }
    motor_run_step(40);
    motor_stop();
}

static void setDebugData(uint8_t *print_data)
{
    for (uint32_t index = 0; index < 48 * 5; ++index)
    {
        //0X55 = 0101 0101 0为白，1为黑
        print_data[index] = 0x55;
    }
}

void testSTB()
{
    //每行48byte 1byte=8bit 384bit
    //48*5=5行
    uint8_t print_data[48*5];
    uint32_t print_len;
    Serial.println("开始打印打印头选通引脚测试\n顺序: 1  2  3  4  5  6");
    print_len = 48*5;
    //设置打印的数据内容
    setDebugData(print_data);
    //通道0打印5行
    start_printing_by_onestb(0, print_data, print_len);
    setDebugData(print_data);
    //通道1打印5行
    start_printing_by_onestb(1, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_onestb(2, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_onestb(3, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_onestb(4, print_data, print_len);
    setDebugData(print_data);
    start_printing_by_onestb(5, print_data, print_len);
    Serial.println("测试完成");
}

void init_printer()
{
    //初始化电机IO
    init_motor();
    //初始化数据引脚、通道引脚
    pinMode(PIN_LAT, OUTPUT);
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_SDA, OUTPUT);
    pinMode(PIN_STB1, OUTPUT);
    pinMode(PIN_STB2, OUTPUT);
    pinMode(PIN_STB3, OUTPUT);
    pinMode(PIN_STB4, OUTPUT);
    pinMode(PIN_STB5, OUTPUT);
    pinMode(PIN_STB6, OUTPUT);
    //加热通道全部关闭
    set_stb_idle();
    //初始化打印电源控制引脚、并关闭电源
    pinMode(PIN_VHEN, OUTPUT);
    digitalWrite(PIN_VHEN, LOW);
    //初始化SPI
    init_spi();
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.printf("setup\n");
  init_printer();
}

void loop() {
  delay(5000);
  testSTB();
}