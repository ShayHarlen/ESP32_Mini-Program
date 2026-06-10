// 图像处理模块 - 缩放、灰度化、二值化、打包

const LINE_BYTES = 48  // 每行48字节 = 384像素

/**
 * 从 canvas 获取图像数据，处理成二值化像素数组
 * @param {Canvas} canvas
 * @param {number} canvasWidth
 * @param {number} canvasHeight
 * @returns {Object} { pixels: Uint8Array, width: number, height: number }
 */
function getCanvasPixelData(canvas, canvasWidth, canvasHeight) {
  const ctx = canvas.getContext('2d')
  const imageData = ctx.getImageData(0, 0, canvasWidth, canvasHeight)
  return {
    data: imageData.data,
    width: canvasWidth,
    height: canvasHeight
  }
}

/**
 * 图像缩放 + 灰度化 + 二值化
 * @param {Object} imageInfo { data, width, height } - RGBA 像素数据
 * @param {number} targetWidth - 目标宽度（384）
 * @param {boolean} useDithering - 是否使用抖动算法
 * @returns {Object} { pixels: Uint8Array(0/1), width, height }
 */
function processImage(imageInfo, targetWidth, useDithering) {
  const { data, width, height } = imageInfo

  // 计算缩放后的高度（保持宽高比）
  const scale = targetWidth / width
  const targetHeight = Math.round(height * scale)

  // 创建浮点灰度数组用于缩放
  const gray = new Float32Array(targetWidth * targetHeight)

  // 双线性插值缩放 + 灰度化
  for (let y = 0; y < targetHeight; y++) {
    for (let x = 0; x < targetWidth; x++) {
      const srcX = x / scale
      const srcY = y / scale

      const x0 = Math.floor(srcX)
      const y0 = Math.floor(srcY)
      const x1 = Math.min(x0 + 1, width - 1)
      const y1 = Math.min(y0 + 1, height - 1)

      const fx = srcX - x0
      const fy = srcY - y0

      // 获取四个邻近像素的灰度值
      const g00 = getGray(data, x0, y0, width)
      const g10 = getGray(data, x1, y0, width)
      const g01 = getGray(data, x0, y1, width)
      const g11 = getGray(data, x1, y1, width)

      // 双线性插值
      gray[y * targetWidth + x] = g00 * (1 - fx) * (1 - fy) +
                                   g10 * fx * (1 - fy) +
                                   g01 * (1 - fx) * fy +
                                   g11 * fx * fy
    }
  }

  // 二值化
  const pixels = new Uint8Array(targetWidth * targetHeight)

  if (useDithering) {
    // Floyd-Steinberg 抖动
    for (let y = 0; y < targetHeight; y++) {
      for (let x = 0; x < targetWidth; x++) {
        const idx = y * targetWidth + x
        const oldVal = gray[idx]
        const newVal = oldVal < 128 ? 0 : 255
        pixels[idx] = newVal === 0 ? 1 : 0
        const error = oldVal - newVal

        if (x + 1 < targetWidth) gray[idx + 1] += error * 7 / 16
        if (y + 1 < targetHeight) {
          if (x - 1 >= 0) gray[(y + 1) * targetWidth + x - 1] += error * 3 / 16
          gray[(y + 1) * targetWidth + x] += error * 5 / 16
          if (x + 1 < targetWidth) gray[(y + 1) * targetWidth + x + 1] += error * 1 / 16
        }
      }
    }
  } else {
    // 简单阈值法
    for (let i = 0; i < gray.length; i++) {
      pixels[i] = gray[i] < 128 ? 1 : 0
    }
  }

  return { pixels, width: targetWidth, height: targetHeight }
}

// 获取像素灰度值（从 RGBA 数据）
function getGray(data, x, y, width) {
  const idx = (y * width + x) * 4
  return data[idx] * 0.299 + data[idx + 1] * 0.587 + data[idx + 2] * 0.114
}

/**
 * 将像素数组打包成字节流（每行48字节）
 * @param {Uint8Array} pixels - 0/1 像素数组
 * @param {number} width - 图像宽度
 * @param {number} height - 图像高度
 * @returns {Uint8Array} 打包后的字节流
 */
function packToBytes(pixels, width, height) {
  const bytesPerRow = width / 8
  const result = new Uint8Array(bytesPerRow * height)

  for (let y = 0; y < height; y++) {
    for (let byteIdx = 0; byteIdx < bytesPerRow; byteIdx++) {
      let byte = 0
      for (let bit = 0; bit < 8; bit++) {
        const x = byteIdx * 8 + bit
        if (pixels[y * width + x] === 1) {
          byte |= (1 << (7 - bit))
        }
      }
      result[y * bytesPerRow + byteIdx] = byte
    }
  }

  return result
}

module.exports = {
  processImage,
  packToBytes,
  LINE_BYTES
}
