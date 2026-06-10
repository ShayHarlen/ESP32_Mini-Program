const ble = require('../../utils/ble')
const imgUtil = require('../../utils/image')
const app = getApp()

Page({
  data: {
    tool: 'draw',           // view / draw / erase
    imageWidth: 0,
    imageHeight: 0,
    canvasDisplayWidth: 0,
    canvasDisplayHeight: 0,
    canPrint: false,
    printing: false
  },

  canvas: null,
  ctx: null,
  pixelData: null,          // Uint8Array, 0=白 1=黑
  originalPixelData: null,  // 用于重置
  undoStack: [],
  displayScale: 1,
  pixelSize: 1,

  onLoad(options) {
    if (options.src) {
      this.loadImage(decodeURIComponent(options.src))
    }
  },

  loadImage(src) {
    wx.showLoading({ title: '图像处理中...' })

    // 先用 wx.getImageInfo 获取图片信息（兼容性最好）
    wx.getImageInfo({
      src: src,
      success: (imgInfo) => {
        const origWidth = imgInfo.width
        const origHeight = imgInfo.height
        console.log('[Editor] 原图尺寸:', origWidth, 'x', origHeight)

        const query = wx.createSelectorQuery()
        query.select('#editorCanvas').fields({ node: true, size: true }).exec((res) => {
          if (!res[0]) {
            wx.hideLoading()
            return
          }

          const canvas = res[0].node
          const ctx = canvas.getContext('2d')
          this.canvas = canvas
          this.ctx = ctx

          const img = canvas.createImage()
          img.onload = () => {
            console.log('[Editor] 图片加载成功:', img.width, 'x', img.height)

            // 第1步：把原图画到 canvas 上，获取像素数据
            canvas.width = img.width
            canvas.height = img.height
            ctx.drawImage(img, 0, 0)
            const imageData = ctx.getImageData(0, 0, img.width, img.height)

            // 第2步：图像处理（缩放到384宽 + 二值化）
            const imageInfo = {
              data: imageData.data,
              width: img.width,
              height: img.height
            }
            const result = imgUtil.processImage(imageInfo, 384, true)
            this.pixelData = result.pixels
            this.originalPixelData = new Uint8Array(result.pixels)
            this.undoStack = []

            const imageWidth = result.width   // 384
            const imageHeight = result.height  // 按比例算出的高度

            // 第3步：计算 canvas 显示尺寸，保持宽高比
            const sysInfo = wx.getSystemInfoSync()
            const maxCanvasWidth = sysInfo.windowWidth - 80
            const maxCanvasHeight = sysInfo.windowHeight * 0.5

            let pixelSize = Math.floor(maxCanvasWidth / imageWidth)
            // 确保高度也不超限
            while (imageHeight * pixelSize > maxCanvasHeight && pixelSize > 1) {
              pixelSize--
            }
            pixelSize = Math.max(pixelSize, 1)

            const canvasDisplayWidth = imageWidth * pixelSize
            const canvasDisplayHeight = imageHeight * pixelSize

            // 设置 canvas 最终尺寸
            canvas.width = canvasDisplayWidth
            canvas.height = canvasDisplayHeight

            this.pixelSize = pixelSize

            this.setData({
              imageWidth,
              imageHeight,
              canvasDisplayWidth,
              canvasDisplayHeight,
              canPrint: true
            })

            // 第4步：绘制黑白像素图
            this.drawPixels()
            wx.hideLoading()
            console.log('[Editor] 处理完成:', imageWidth, 'x', imageHeight, '像素大小:', pixelSize)
          }

          img.onerror = (err) => {
            console.log('[Editor] 图片加载失败:', err)
            wx.hideLoading()
            wx.showModal({
              title: '加载失败',
              content: '无法加载图片',
              showCancel: false,
              success: () => wx.navigateBack()
            })
          }

          img.src = imgInfo.path
        })
      },
      fail: (err) => {
        console.log('[Editor] getImageInfo失败:', err)
        wx.hideLoading()
        wx.showModal({
          title: '加载失败',
          content: '无法读取图片信息',
          showCancel: false,
          success: () => wx.navigateBack()
        })
      }
    })
  },

  // 在 canvas 上绘制像素
  drawPixels() {
    const { imageWidth, imageHeight } = this.data
    const ctx = this.ctx
    const ps = this.pixelSize

    // 白色背景
    ctx.fillStyle = '#ffffff'
    ctx.fillRect(0, 0, imageWidth * ps, imageHeight * ps)

    // 绘制黑色像素
    ctx.fillStyle = '#000000'
    for (let y = 0; y < imageHeight; y++) {
      for (let x = 0; x < imageWidth; x++) {
        if (this.pixelData[y * imageWidth + x] === 1) {
          ctx.fillRect(x * ps, y * ps, ps, ps)
        }
      }
    }

    // 绘制网格线（如果像素够大）
    if (ps >= 4) {
      ctx.strokeStyle = '#e0e0e0'
      ctx.lineWidth = 0.5
      for (let x = 0; x <= imageWidth; x++) {
        ctx.beginPath()
        ctx.moveTo(x * ps, 0)
        ctx.lineTo(x * ps, imageHeight * ps)
        ctx.stroke()
      }
      for (let y = 0; y <= imageHeight; y++) {
        ctx.beginPath()
        ctx.moveTo(0, y * ps)
        ctx.lineTo(imageWidth * ps, y * ps)
        ctx.stroke()
      }
    }
  },

  // 触摸事件处理
  onTouchStart(e) {
    this.touching = true
    this.handleTouch(e)
  },

  onTouchMove(e) {
    if (this.touching) {
      this.handleTouch(e)
    }
  },

  onTouchEnd() {
    this.touching = false
    if (this.data.tool !== 'view' && this.undoStack.length > 0) {
      // 触摸结束时压入撤销栈
    }
  },

  handleTouch(e) {
    if (this.data.tool === 'view') return

    const touch = e.touches[0]
    const query = wx.createSelectorQuery()
    query.select('#editorCanvas').boundingClientRect((rect) => {
      if (!rect) return

      const x = Math.floor((touch.clientX - rect.left) / this.pixelSize)
      const y = Math.floor((touch.clientY - rect.top) / this.pixelSize)
      const { imageWidth, imageHeight } = this.data

      if (x < 0 || x >= imageWidth || y < 0 || y >= imageHeight) return

      const idx = y * imageWidth + x
      let newVal

      if (this.data.tool === 'draw') {
        newVal = 1
      } else if (this.data.tool === 'erase') {
        newVal = 0
      }

      if (this.pixelData[idx] !== newVal) {
        // 保存撤销点
        this.undoStack.push({
          idx,
          oldVal: this.pixelData[idx]
        })
        if (this.undoStack.length > 100) this.undoStack.shift()

        this.pixelData[idx] = newVal
        this.drawPixels()
      }
    }).exec()
  },

  setTool(e) {
    this.setData({ tool: e.currentTarget.dataset.tool })
  },

  undoAction() {
    if (this.undoStack.length === 0) return
    const action = this.undoStack.pop()
    this.pixelData[action.idx] = action.oldVal
    this.drawPixels()
  },

  resetImage() {
    if (!this.originalPixelData) return
    this.pixelData = new Uint8Array(this.originalPixelData)
    this.undoStack = []
    this.drawPixels()
  },

  chooseNewImage() {
    wx.chooseMedia({
      count: 1,
      mediaType: ['image'],
      success: (res) => {
        this.loadImage(res.tempFiles[0].tempFilePath)
      }
    })
  },

  async startPrint() {
    if (!this.data.canPrint || this.data.printing) return
    if (!app.globalData.bleConnected) {
      wx.showModal({
        title: '未连接',
        content: '请先连接打印机',
        showCancel: false
      })
      return
    }

    this.setData({ printing: true })
    wx.showLoading({ title: '打印中...' })

    try {
      const { imageWidth, imageHeight } = this.data
      const binaryData = imgUtil.packToBytes(this.pixelData, imageWidth, imageHeight)
      await ble.printImage(binaryData, imageHeight)
      wx.hideLoading()
      wx.showToast({ title: '打印完成', icon: 'success' })
    } catch (err) {
      wx.hideLoading()
      wx.showModal({
        title: '打印失败',
        content: err.message,
        showCancel: false
      })
    } finally {
      this.setData({ printing: false })
    }
  }
})
