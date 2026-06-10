const api = require('../../utils/api')

Page({
  data: {
    mode: 'text2img',     // text2img 或 img2img
    prompt: '',
    refImagePath: '',       // 图生图的参考图片路径
    refImageBase64: '',     // 参考图片的 base64
    selectedSize: '1024x1024',
    generating: false,
    resultImage: '',
    sizes: [
      { label: '1:1', value: '1024x1024' },
      { label: '16:9', value: '1792x1024' },
      { label: '9:16', value: '1024x1792' },
      { label: '4:3', value: '1024x768' }
    ]
  },

  switchMode(e) {
    this.setData({
      mode: e.currentTarget.dataset.mode,
      resultImage: ''
    })
  },

  onInput(e) {
    this.setData({ prompt: e.detail.value })
  },

  selectSize(e) {
    this.setData({ selectedSize: e.currentTarget.dataset.value })
  },

  chooseRefImage() {
    wx.chooseMedia({
      count: 1,
      mediaType: ['image'],
      success: async (res) => {
        const path = res.tempFiles[0].tempFilePath
        try {
          const base64 = await api.fileToBase64(path)
          this.setData({
            refImagePath: path,
            refImageBase64: base64
          })
        } catch (err) {
          wx.showToast({ title: '读取图片失败', icon: 'none' })
        }
      }
    })
  },

  removeRefImage() {
    this.setData({ refImagePath: '', refImageBase64: '' })
  },

  async generate() {
    if (!this.data.prompt || this.data.generating) return

    this.setData({ generating: true, resultImage: '' })
    wx.showLoading({ title: '生成中...' })

    try {
      let imageUrl

      if (this.data.mode === 'text2img') {
        imageUrl = await api.textToImage(this.data.prompt, this.data.selectedSize)
      } else {
        if (!this.data.refImageBase64) {
          wx.hideLoading()
          wx.showToast({ title: '请先上传参考图片', icon: 'none' })
          this.setData({ generating: false })
          return
        }
        imageUrl = await api.imageToImage(this.data.refImageBase64, this.data.prompt)
      }

      wx.hideLoading()

      // 如果是网络 URL，下载到本地
      if (imageUrl.startsWith('http')) {
        const localPath = await api.downloadImage(imageUrl)
        this.setData({ resultImage: localPath, generating: false })
      } else {
        this.setData({ resultImage: imageUrl, generating: false })
      }
    } catch (err) {
      wx.hideLoading()
      this.setData({ generating: false })
      wx.showModal({
        title: '生成失败',
        content: err.message,
        showCancel: false
      })
    }
  },

  saveImage() {
    if (!this.data.resultImage) return
    wx.saveImageToPhotosAlbum({
      filePath: this.data.resultImage,
      success: () => wx.showToast({ title: '已保存', icon: 'success' }),
      fail: () => wx.showToast({ title: '保存失败', icon: 'none' })
    })
  },

  printResult() {
    if (!this.data.resultImage) return
    wx.navigateTo({
      url: '/pages/editor/editor?src=' + encodeURIComponent(this.data.resultImage)
    })
  }
})
