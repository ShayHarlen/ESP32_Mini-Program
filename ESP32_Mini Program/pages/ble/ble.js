const app = getApp()
const ble = require('../../utils/ble')

Page({
  data: {
    connected: false,
    connecting: false,
    battery: 0,
    temperature: 0,
    hasNoPaper: false
  },

  onShow() {
    this.setData({ connected: app.globalData.bleConnected })
    if (this.data.connected) {
      this.refreshStatus()
    }
  },

  onUnload() {
    // 离开页面不断开连接，保持后台连接
  },

  async startConnect() {
    if (this.data.connecting) return
    this.setData({ connecting: true })

    try {
      await ble.initBLE()
      await ble.connectPrinter()
      app.globalData.bleConnected = true
      this.setData({ connected: true, connecting: false })
      wx.showToast({ title: '连接成功', icon: 'success' })
      this.refreshStatus()
    } catch (err) {
      this.setData({ connecting: false })
      wx.showModal({
        title: '连接失败',
        content: err.message,
        showCancel: false
      })
    }
  },

  async refreshStatus() {
    try {
      const status = await ble.readStatus()
      this.setData({
        battery: status.battery,
        temperature: status.temperature,
        hasNoPaper: status.hasNoPaper
      })
    } catch (err) {
      console.error('读取状态失败:', err)
    }
  },

  disconnectDevice() {
    ble.disconnect()
    app.globalData.bleConnected = false
    this.setData({
      connected: false,
      battery: 0,
      temperature: 0,
      hasNoPaper: false
    })
    wx.showToast({ title: '已断开', icon: 'success' })
  }
})
