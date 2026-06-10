const app = getApp()

Page({
  data: {
    bleConnected: false
  },

  onShow() {
    this.setData({ bleConnected: app.globalData.bleConnected })
  },

  goToBle() {
    wx.navigateTo({ url: '/pages/ble/ble' })
  },

  goToAI() {
    wx.switchTab({ url: '/pages/ai/ai' })
  },

  chooseImage() {
    wx.chooseMedia({
      count: 1,
      mediaType: ['image'],
      success: (res) => {
        const tempFilePath = res.tempFiles[0].tempFilePath
        wx.navigateTo({
          url: '/pages/editor/editor?src=' + encodeURIComponent(tempFilePath)
        })
      }
    })
  }
})
