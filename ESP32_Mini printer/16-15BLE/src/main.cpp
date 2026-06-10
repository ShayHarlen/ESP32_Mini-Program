#include <Arduino.h>
#include <BLEDevice.h>
#include <BLE2902.h>

#define BLE_NAME "Mini-Printer"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"         // 自定义打印服务UUID
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // 自定义打印特征UUID

bool bleConnected = false;

BLECharacteristic *pCharacteristic;
#define PIN_LED 18
unsigned char level;

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

void ble_report(){
    if (bleConnected == true){
        uint8_t status[4];
        status[0] = 0;
        status[1] = 1;
        status[2] = 2;
        status[3] = 3;
        pCharacteristic->setValue((uint8_t*)&status,sizeof(status));
        pCharacteristic->notify();
    }
}

#define PIN_BATTERY_ADC 34
int readvoltage(){
  int data=0;
  data = analogReadMilliVolts(PIN_BATTERY_ADC);
  return data;
}
/*电池满电4.2 有电3.3 但电路是分压电路 读取的是一般的电压 所以要乘2 同时readvoltage的单位是mv */
void Batterylevel(){
   level=map(readvoltage()*2,3300,4200,0,100);
   Serial.printf("电量=%d\n",level);
}

class bleServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        // 🔴【核心战区】函数外壳是官方给的，但里面的动作是你写的！
        bleConnected = true; // 记录状态
        Serial.println("现在有设备接入~");
        
        // 💡 思考：在这里你还需要绝对掌控什么？
        // 如果这是一个打印机，有人连上了，你要不要写代码让板子上的 LED 指示灯变成常亮（代表已连接）？
        // 要不要蜂鸣器 "滴" 一声提醒用户？这些全是你亲自动手写的地方。
        led_status(0);
    }

    void onDisconnect(BLEServer *pServer)
    {
        // 🔴【核心战区】断开连接后的急救措施。
        bleConnected = false;
        Serial.println("现在有设备断开连接~");
        
        // 🟢【拿来主义】重新开启广播。这句API是调用的。
        pServer->startAdvertising(); 
        
        // 💡 思考：除了重新广播，你还需要亲自动手写什么？
        // 比如：如果打印机正在打印，手机突然断开了，你要不要在这里写代码让电机立刻停转，防止机器把一整卷纸全吐出来？
        led_status(1);
    }
};

class bleCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *pCharacteristic)
    { 
        // 🔴【核心战区】当手机请求读取数据时触发。
        Serial.println("触发读取事件");
        // 💡 思考：你不能只打印一句话。你需要亲自动手写代码，把打印机当前的状态（比如电池电量）
        // 用 pCharacteristic->setValue() 塞进去，让手机读走。
      Batterylevel();
      String message = "Level: " + String(level) + "%";
      pCharacteristic->setValue(message.c_str());
    }

    void onWrite(BLECharacteristic *pCharacteristic)
    { 
        // 🟢【拿来主义】这是官方提供的“取快递”工具。
        // 这两行代码的作用是：问快递有多重（length），把快递拆开拿出来（pdata）。你会调用就行。
        size_t length = pCharacteristic->getLength();
        uint8_t *pdata = pCharacteristic->getData();
        Serial.printf("触发写入事件 length=%d \n", length);
        
        // 🚀🚀🚀 【最高级别的核心战区 - 必须你亲自动手写】 🚀🚀🚀
        // 从这一行往下，就是你的秀场了！没有任何现成的代码可以抄，因为只有你知道 pdata 里装的是什么。
        
        // 比如，高手会在这里写类似这样的解析逻辑：
        /*
        if (pdata[0] == 0x01) {
            // 解析出：这是一个纯文本打印命令
            Serial.printf("收到文本: %s\n", (char*)pdata);
            // 接着调用你写的“电机走纸函数”和“加热头打印函数”
        } 
        else if (pdata[0] == 0x02) {
            // 解析出：这是一张图片的二进制数据，开始写入图片缓存区...
        }
        */
    }
};

void init_ble()
{
    // 🟢【拿来主义】打开底层蓝牙芯片的电源，把名字挂出去。
    // 你只需要知道这行代码启动了ESP32的射频天线。
    BLEDevice::init(BLE_NAME); 

    // 🟢【拿来主义】向系统申请一块内存，建立一个蓝牙服务器节点。
    BLEServer *pServer = BLEDevice::createServer();  
    
    // 🟢【拿来主义】但这是个“桥梁”。
    // 这行代码本身不用你改，但它把底层连接状态与你写的 `bleServerCallbacks` 绑定在了一起。
    //交给pServer的函数，就是连接或者断开时会执行的
    pServer->setCallbacks(new bleServerCallbacks()); 

    // 🟢【拿来主义】建立服务窗口。
    BLEService *pService = pServer->createService(SERVICE_UUID); 
    
    // 🔴【半掌控状态】虽然代码格式是死套路，但里面的参数是你定的！
    pCharacteristic = pService->createCharacteristic(            
        CHARACTERISTIC_UUID, // 🔴 你必须亲手分配好UUID，这是你的通信协议基础。
        // 🔴 你必须亲手决定这个通道有什么权限。
        // 如果你忘了加 PROPERTY_WRITE，手机发数据就会报错被拒收。
            BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_WRITE);
            
    // 🟢【拿来主义】同上，绑定业务回调。
    //交给pCharacteristic的函数，是写入或者读取或者通知时会执行的
    pCharacteristic->setCallbacks(new bleCharacteristicCallbacks());

    // 🟢【拿来主义】开门营业。
    pService->start(); 
    // 🟢【拿来主义】开启广播，让手机能搜到你。
    BLEDevice::startAdvertising();
}

void setup() {
  // 初始化按键引脚为输入模式
  Serial.begin(115200);
  Serial.printf("setup\n");
  init_ble();
  pinMode(18,OUTPUT);
}

void loop() {
  delay(5000);

}