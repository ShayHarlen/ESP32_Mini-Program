// BLE 蓝牙通信模块 - 与 ESP32 Mini-Printer 通信

const PRINTER_NAME = 'Mini-Printer'
const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c331914b'
const CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b26a8'

// 命令类型（与 ESP32 端一致）
const CMD_PRINT_LINE = 0x02
const CMD_PRINT_START = 0x03

let deviceId = ''
let serviceId = ''
let characteristicId = ''

// 初始化蓝牙适配器（先关闭再打开，清除残留状态）
function initBLE() {
  return new Promise((resolve, reject) => {
    wx.closeBluetoothAdapter({
      complete: () => {
        wx.openBluetoothAdapter({
          success: () => {
            console.log('[BLE] 蓝牙适配器已初始化')
            resolve()
          },
          fail: (err) => reject(new Error('蓝牙初始化失败: ' + err.errMsg))
        })
      }
    })
  })
}

// 搜索并连接打印机
function connectPrinter() {
  return new Promise((resolve, reject) => {
    wx.startBluetoothDevicesDiscovery({
      services: [],
      allowDuplicatesKey: false,
      success: () => {
        console.log('[BLE] 开始搜索蓝牙设备...')

        wx.onBluetoothDeviceFound((res) => {
          for (const device of res.devices) {
            console.log('[BLE] 发现设备:', device.name || device.localName, '| deviceId:', device.deviceId)

            const deviceName = device.name || device.localName || ''
            if (deviceName.indexOf('Mini') !== -1 || deviceName.indexOf('Printer') !== -1 || deviceName.indexOf('printer') !== -1) {
              console.log('[BLE] 匹配到打印机:', deviceName, 'deviceId:', device.deviceId)
              wx.stopBluetoothDevicesDiscovery()
              deviceId = device.deviceId
              // 等500ms再连接，避免蓝牙状态冲突
              setTimeout(() => {
                connectToDevice().then(resolve).catch(reject)
              }, 500)
              return
            }
          }
        })

        // 15秒超时
        setTimeout(() => {
          wx.stopBluetoothDevicesDiscovery()
          reject(new Error('未找到打印机，请确认设备已开机且蓝牙已开启'))
        }, 15000)
      },
      fail: (err) => reject(new Error('搜索失败: ' + err.errMsg))
    })
  })
}

// 连接设备并获取服务
function connectToDevice() {
  return new Promise((resolve, reject) => {
    console.log('[BLE] 正在连接设备:', deviceId)
    // 先关闭可能存在的旧连接
    wx.closeBLEConnection({ deviceId })
    wx.createBLEConnection({
      deviceId,
      success: () => {
        console.log('[BLE] 连接成功，正在协商MTU...')
        wx.setBLEMTU({
          deviceId,
          mtu: 512,
          success: () => console.log('[BLE] MTU协商成功: 512'),
          fail: (err) => console.log('[BLE] MTU协商失败(不影响连接):', err.errMsg)
        })
        // 等500ms让MTU协商完成，再获取服务
        setTimeout(() => {
          wx.getBLEDeviceServices({
            deviceId,
            success: (res) => {
              console.log('[BLE] 获取到', res.services.length, '个服务')
              for (const service of res.services) {
                console.log('[BLE] 服务UUID:', service.uuid)
                if (service.uuid.toLowerCase() === SERVICE_UUID) {
                  serviceId = service.uuid
                  console.log('[BLE] 匹配到打印服务')
                  getCharacteristic().then(resolve).catch(reject)
                  return
                }
              }
              reject(new Error('未找到打印服务，请检查ESP32是否正常运行'))
            },
            fail: (err) => reject(new Error('获取服务失败: ' + err.errMsg))
          })
        }, 500)
      },
      fail: (err) => {
        console.log('[BLE] 连接失败:', err.errCode, err.errMsg)
        reject(new Error('连接失败(' + err.errCode + '): ' + err.errMsg))
      }
    })
  })
}

// 获取特征值
function getCharacteristic() {
  return new Promise((resolve, reject) => {
    wx.getBLEDeviceCharacteristics({
      deviceId,
      serviceId,
      success: (res) => {
        for (const char of res.characteristics) {
          if (char.uuid.toLowerCase() === CHARACTERISTIC_UUID) {
            characteristicId = char.uuid
            resolve({ deviceId, serviceId, characteristicId })
            return
          }
        }
        reject(new Error('未找到特征值'))
      },
      fail: (err) => reject(new Error('获取特征值失败: ' + err.errMsg))
    })
  })
}

// 读取打印机状态（通过 onRead）
let statusListener = null

function readStatus() {
  return new Promise((resolve, reject) => {
    // 移除旧监听器，防止重复注册
    if (statusListener) {
      wx.offBLECharacteristicValueChange(statusListener)
      statusListener = null
    }

    statusListener = (res) => {
      const data = new Uint8Array(res.value)
      if (data.length >= 7 && data[0] === 0x02) {
        const hasNoPaper = data[1] === 1
        const battery = data[2]
        const tempBytes = new Uint8Array(data.slice(3, 7))
        const tempView = new DataView(tempBytes.buffer)
        const temperature = tempView.getFloat32(0, true)
        resolve({
          hasNoPaper,
          battery,
          temperature: Math.round(temperature * 10) / 10
        })
      }
    }

    wx.onBLECharacteristicValueChange(statusListener)

    wx.readBLECharacteristicValue({
      deviceId,
      serviceId,
      characteristicId,
      fail: (err) => reject(new Error('读取失败: ' + err.errMsg))
    })
  })
}

// 写入数据到特征值
function writeData(buffer) {
  return new Promise((resolve, reject) => {
    wx.writeBLECharacteristicValue({
      deviceId,
      serviceId,
      characteristicId,
      value: buffer,
      success: () => resolve(),
      fail: (err) => reject(new Error('写入失败: ' + err.errMsg))
    })
  })
}

// 发送打印任务
async function printImage(binaryData, totalLines) {
  // 第1步：发送打印开始命令 [0x03, 行数高8位, 行数低8位]
  const startCmd = new ArrayBuffer(3)
  const startView = new Uint8Array(startCmd)
  startView[0] = CMD_PRINT_START
  startView[1] = (totalLines >> 8) & 0xFF
  startView[2] = totalLines & 0xFF
  await writeData(startCmd)

  // 等待 ESP32 处理
  await sleep(100)

  // 第2步：逐行发送数据
  for (let i = 0; i < totalLines; i++) {
    const lineData = binaryData.slice(i * 48, (i + 1) * 48)
    const lineCmd = new ArrayBuffer(1 + 48)
    const lineView = new Uint8Array(lineCmd)
    lineView[0] = CMD_PRINT_LINE
    lineView.set(lineData, 1)
    await writeData(lineCmd)

    // 每行之间稍微等待，防止 BLE 拥塞
    if (i % 10 === 9) {
      await sleep(50)
    }
  }
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms))
}

// 断开连接
function disconnect() {
  if (deviceId) {
    wx.closeBLEConnection({ deviceId })
  }
  wx.closeBluetoothAdapter({
    complete: () => {
      console.log('[BLE] 蓝牙适配器已关闭')
    }
  })
  deviceId = ''
  serviceId = ''
  characteristicId = ''
}

module.exports = {
  initBLE,
  connectPrinter,
  readStatus,
  printImage,
  disconnect,
  isConnected: () => !!deviceId
}
