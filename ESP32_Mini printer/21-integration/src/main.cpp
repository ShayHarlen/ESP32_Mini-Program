  #include <Arduino.h>
  #include <BLEDevice.h>
  #include <BLE2902.h>
  
  // 来自16-15BLE
  #define BLE_NAME "Mini-Printer"
  #define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
  #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

  // 来自print_deom001
  #define PIN_LED 18

  // 来自17-16Paper Lack
  #define PIN_PAPER 35

  // 来自18-17temperature
  #define PIN_temperature 36

  // 来自21-integration原来的
  #define PIN_VHEN 17

  // 命令类型定义
  #define CMD_READ_STATUS  0x01  // 小程序发来：读状态
  #define CMD_PRINT_LINE   0x02  // 小程序发来：一行打印数据
  #define CMD_PRINT_START  0x03  // 小程序发来：开始打印

  // 打印缓冲区
  #define MAX_LINES 500          // 最多存500行
  #define LINE_BYTES 48          // 每行48字节(384像素)

  uint8_t printBuffer[MAX_LINES * LINE_BYTES];  // 打印缓冲区，约24KB
  int totalLines = 0;           // 小程序说要发多少行
  int receivedLines = 0;        // 目前收到了多少行
  bool isPrinting = false;      // 是否正在打印


 void ble_report();

  void LEDFLASH(int ms)
  {
      digitalWrite(PIN_LED, LOW);
      delay(ms);
      digitalWrite(PIN_LED, HIGH);
      delay(ms);
  }

  void led_status(int status)
  {
      if (status == 0)
          digitalWrite(PIN_LED, LOW);
      else if (status == 1)
          digitalWrite(PIN_LED, HIGH);
      else if (status == 2)
          LEDFLASH(250);
      else if (status == 3)
          LEDFLASH(1000);
  }
  
    bool read_paper_status()
  {
      if (digitalRead(PIN_PAPER) == LOW)
          return true;   // 有纸
      else
          return false;  // 缺纸
  }

    float Calculate_temperature()
  {
      float voltage = analogReadMilliVolts(PIN_temperature);
      float resistance = (voltage * 10) / (3.3 - voltage / 1000);
      float temperature = 1 / (log(resistance / 30000) / 3950 + 1 / 298.15) - 273.15;
      return temperature;
  }

  #define PIN_BATTERY_ADC 34
   // ==================== 电机部分 ====================
  #define PIN_MOTOR_AP 23
  #define PIN_MOTOR_AM 22
  #define PIN_MOTOR_BP 21
  #define PIN_MOTOR_BM 19

  #define MOTOR_WATI_TIME 2

  uint8_t motor_table[8][4] = {
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

  void motor_run()
  {
      digitalWrite(PIN_MOTOR_AP, motor_table[motor_pos][0]);
      digitalWrite(PIN_MOTOR_AM, motor_table[motor_pos][1]);
      digitalWrite(PIN_MOTOR_BP, motor_table[motor_pos][2]);
      digitalWrite(PIN_MOTOR_BM, motor_table[motor_pos][3]);
      motor_pos++;
      if (motor_pos >= 8)
          motor_pos = 0;
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
              motor_pos = 0;
          delay(MOTOR_WATI_TIME);
          steps--;
      }
  }

  void motor_stop()
  {
      digitalWrite(PIN_MOTOR_AP, 0);
      digitalWrite(PIN_MOTOR_AM, 0);
      digitalWrite(PIN_MOTOR_BP, 0);
      digitalWrite(PIN_MOTOR_BM, 0);
  }

  void init_motor()
  {
      pinMode(PIN_MOTOR_AP, OUTPUT);
      pinMode(PIN_MOTOR_AM, OUTPUT);
      pinMode(PIN_MOTOR_BP, OUTPUT);
      pinMode(PIN_MOTOR_BM, OUTPUT);
      motor_stop();
  }

   // ==================== SPI部分 ====================
  #include <SPI.h>

  #define PIN_SCK 15
  #define PIN_SDA 13

  SPIClass hspi = SPIClass(HSPI);
  SPISettings printerSPISettings = SPISettings(1000000, SPI_MSBFIRST, SPI_MODE0);

  void spiCommand(uint8_t *data_buffer, uint8_t data_len)
  {
      hspi.beginTransaction(printerSPISettings);
      hspi.transfer(data_buffer, data_len);
      hspi.endTransaction();
  }

  void init_spi()
  {
      hspi.begin(PIN_SCK, 16, PIN_SDA, -1);  // SCLK, MISO, MOSI, SS
      hspi.setFrequency(2000000);
  }
  //
  // ==================== 打印头部分 ====================
  #define PIN_LAT 12
  #define PIN_STB1 26
  #define PIN_STB2 27
  #define PIN_STB3 14
  #define PIN_STB4 32
  #define PIN_STB5 33
  #define PIN_STB6 25

  #define PRINT_TIME 1700
  #define PRINT_END_TIME 200
  #define LAT_TIME 1

  void set_stb_idle()
  {
      digitalWrite(PIN_STB1, LOW);
      digitalWrite(PIN_STB2, LOW);
      digitalWrite(PIN_STB3, LOW);
      digitalWrite(PIN_STB4, LOW);
      digitalWrite(PIN_STB5, LOW);
      digitalWrite(PIN_STB6, LOW);
  }

  void init_printing()
  {
      set_stb_idle();
      digitalWrite(PIN_LAT, HIGH);
      digitalWrite(PIN_VHEN, HIGH);  // 打印头电源开
  }

  void stop_printing()
  {
      digitalWrite(PIN_VHEN, LOW);   // 打印头电源关
      set_stb_idle();
      digitalWrite(PIN_LAT, HIGH);
  }

  void send_one_line_data(uint8_t *data)
  {
      spiCommand(data, 48);
      digitalWrite(PIN_LAT, LOW);
      delayMicroseconds(LAT_TIME);
      digitalWrite(PIN_LAT, HIGH);
  }

  void run_stb(uint8_t now_stb_num)
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

  void move_and_run_stb(uint8_t stbnum)
  {
      motor_run();
      run_stb(stbnum);
      motor_run_step(3);
  }

    void init_printer()
  {
      init_motor();

      pinMode(PIN_LAT, OUTPUT);
      pinMode(PIN_STB1, OUTPUT);
      pinMode(PIN_STB2, OUTPUT);
      pinMode(PIN_STB3, OUTPUT);
      pinMode(PIN_STB4, OUTPUT);
      pinMode(PIN_STB5, OUTPUT);
      pinMode(PIN_STB6, OUTPUT);

      set_stb_idle();

      init_spi();
  }

   void task_print(void *pvParameters)
  {
      for (;;)
      {
          // 没收到数据 或 正在打印，就一直等
          if (totalLines == 0 || receivedLines < totalLines || isPrinting)
          {
              vTaskDelay(100 / portTICK_PERIOD_MS);
              continue;
          }

          // 数据收齐了，开始打印
          isPrinting = true;
          Serial.printf("开始打印 %d 行\n", totalLines);

          // 打印头初始化（开电源）
          init_printing();

          uint8_t *ptr = printBuffer;
          for (int i = 0; i < totalLines; i++)
          {
              // 第1步：发送一行数据到打印头
              send_one_line_data(ptr);

              // 第2步：纸不动，依次加热6个通道（每个通道加热64个点）
              for (int stb = 0; stb < 6; stb++)
              {
                  run_stb(stb);
              }

              // 第3步：6个通道全部加热完，再移动电机走下一行
              motor_run_step(4);

              ptr += LINE_BYTES;
              Serial.printf("打印 %d/%d\n", i + 1, totalLines);
          }

          // 打印完毕，多走40步把纸送出来
          motor_run_step(40);
          motor_stop();
          stop_printing();

          // 重置状态
          totalLines = 0;
          receivedLines = 0;
          isPrinting = false;

          Serial.println("打印完成");
      }
  }

  //读电压
  int readvoltage()
  {
      int data = analogReadMilliVolts(PIN_BATTERY_ADC);
      return data;
  }
  //根据电压算电量
  uint8_t read_battery_level()
  {
      // 电池满电4.2V 有电3.3V 分压电路读一半 所以乘2
      uint8_t level = map(readvoltage() * 2, 3300, 4200, 0, 100);
      return level;
  }
  
  bool bleConnected = false;
  BLECharacteristic *pCharacteristic;

    class bleServerCallbacks : public BLEServerCallbacks
  {
      void onConnect(BLEServer *pServer)
      {
          bleConnected = true;
          Serial.println("蓝牙已连接");
          led_status(1);  // ← 改这里：原来16-15BLE写的是0，你要改成1（亮）
      }

      void onDisconnect(BLEServer *pServer)
      {
          bleConnected = false;
          Serial.println("蓝牙已断开");
          pServer->startAdvertising();
          led_status(0);  // ← 改这里：原来写的是1，你要改成0（灭）
      }
  };

    class bleCharacteristicCallbacks : public BLECharacteristicCallbacks
  {
      void onRead(BLECharacteristic *pCharacteristic)
      {
          Serial.println("触发读取事件");
          ble_report();
      }

  void onWrite(BLECharacteristic *pCharacteristic)
  {
      size_t length = pCharacteristic->getLength();
      uint8_t *pdata = pCharacteristic->getData();

      if (length < 1) return;  // 数据为空，不处理

      switch (pdata[0])
      {
          case CMD_READ_STATUS:  // 0x01 小程序来读状态
              Serial.println("收到：读状态命令");
              ble_report();      // 把状态数据setValue进去
              break;

          case CMD_PRINT_LINE:   // 0x02 小程序发来一行打印数据
              // pdata[1]开始才是像素数据，要48字节
              if (length >= 1 + LINE_BYTES && receivedLines < totalLines)
              {
                  // 把48字节存到缓冲区对应位置
                  memcpy(&printBuffer[receivedLines * LINE_BYTES], &pdata[1], LINE_BYTES);
                  receivedLines++;
                  Serial.printf("收到第 %d/%d 行\n", receivedLines, totalLines);
              }
              break;

          case CMD_PRINT_START:  // 0x03 小程序说要打印了
              if (length >= 3)
              {
                  // 字节1是行数高8位，字节2是行数低8位，合起来就是总行数
                  totalLines = (pdata[1] << 8) | pdata[2];
                  receivedLines = 0;
                  isPrinting = false;
                  Serial.printf("准备接收 %d 行打印数据\n", totalLines);
              }
              break;

          default:
              Serial.printf("未知命令: 0x%02X\n", pdata[0]);
              break;
      }
  }
  };

    void init_ble()
  {
      BLEDevice::setMTU(512);  // 协商最大传输单元为512字节
      BLEDevice::init(BLE_NAME);
      BLEServer *pServer = BLEDevice::createServer();
      pServer->setCallbacks(new bleServerCallbacks());
      BLEService *pService = pServer->createService(SERVICE_UUID);
      pCharacteristic = pService->createCharacteristic(
          CHARACTERISTIC_UUID,
          BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_WRITE);
      pCharacteristic->setCallbacks(new bleCharacteristicCallbacks());
      pService->start();
      BLEDevice::startAdvertising();
  }

  

  void ble_report()
  {
      // 按协议打包：0x02 + 缺纸 + 电量 + 温度(4字节) = 共7字节
      uint8_t data[7];

      data[0] = 0x02;                          // 命令类型：状态响应
      data[1] = read_paper_status() ? 0 : 1;   // 缺纸状态：0=有纸，1=缺纸
      data[2] = read_battery_level();           // 电量：0~100

      float temp = Calculate_temperature();
      memcpy(&data[3], &temp, 4);              // 温度：float占4字节

      pCharacteristic->setValue(data, 7);
  }

    void setup()
  {
      Serial.begin(115200);
      Serial.println("系统启动");

      // 初始化引脚
      pinMode(PIN_LED, OUTPUT);
      pinMode(PIN_PAPER, INPUT);
      pinMode(PIN_VHEN, OUTPUT);
      digitalWrite(PIN_VHEN, LOW);  // 关闭打印头电源，安全第一

      // 未连接蓝牙，灯不亮
      led_status(0);

      // 启动蓝牙
      init_ble();

      Serial.println("蓝牙已启动，等待连接...");

      init_printer();

      xTaskCreate(
      task_print,     // 任务函数
      "TaskPrint",    // 任务名
      1024 * 10,      // 栈大小
      NULL,           // 参数
      1,              // 优先级
      NULL            // 句柄
        );
  }

  void loop()
  {
      delay(1000);
  }