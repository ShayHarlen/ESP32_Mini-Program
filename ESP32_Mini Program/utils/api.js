// AI 图像生成 API 模块

const BASE_URL = 'https://api.harlen.it.com'
const API_KEY = 'sk-350cbc7c68fbfc0fa94708484296220695f710186b3dd35fe6ee16925f9e7aa5'

/**
 * 文生图：根据文字描述生成图片
 * @param {string} prompt - 图片描述
 * @param {string} size - 图片尺寸，如 "1024x1024"
 * @returns {Promise<string>} 生成的图片 URL 或 base64
 */
function textToImage(prompt, size) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: BASE_URL + '/v1/images/generations',
      method: 'POST',
      timeout: 300000,  // 5分钟超时，AI生图需要较长时间
      header: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ' + API_KEY
      },
      data: {
        model: 'gpt-image-2',
        prompt: prompt,
        n: 1,
        size: size || '1024x1024'
      },
      success: (res) => {
        console.log('[API] 文生图响应:', res.statusCode, JSON.stringify(res.data).substring(0, 200))
        if (res.statusCode === 200 && res.data) {
          if (res.data.data && res.data.data[0]) {
            const item = res.data.data[0]
            if (item.url) {
              // 替换内部地址为公网地址（与后端逻辑一致）
              const publicUrl = item.url.replace('http://43.128.132.88:3003', 'https://image.harlen.it.com')
              console.log('[API] 图片URL:', publicUrl)
              resolve(publicUrl)
            } else if (item.b64_json) {
              resolve('data:image/png;base64,' + item.b64_json)
            } else {
              reject(new Error('返回数据格式异常'))
            }
          } else {
            reject(new Error('返回数据为空'))
          }
        } else {
          const errMsg = res.data && res.data.error ? res.data.error.message : ('HTTP ' + res.statusCode)
          console.log('[API] 文生图失败:', errMsg)
          reject(new Error('请求失败: ' + errMsg))
        }
      },
      fail: (err) => reject(new Error('网络请求失败: ' + err.errMsg))
    })
  })
}

/**
 * 图生图：根据参考图片和描述生成新图片
 * @param {string} imageBase64 - 参考图片的 base64
 * @param {string} prompt - 图片描述
 * @returns {Promise<string>} 生成的图片 URL 或 base64
 */
function imageToImage(imageBase64, prompt) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: BASE_URL + '/v1/images/edits',
      method: 'POST',
      timeout: 300000,
      header: {
        'Content-Type': 'application/json',
        'Authorization': 'Bearer ' + API_KEY
      },
      data: {
        model: 'gpt-image-2',
        image: imageBase64,
        prompt: prompt,
        n: 1,
        size: '1024x1024'
      },
      success: (res) => {
        if (res.statusCode === 200 && res.data && res.data.data && res.data.data[0]) {
          const item = res.data.data[0]
          if (item.url) {
            const publicUrl = item.url.replace('http://43.128.132.88:3003', 'https://image.harlen.it.com')
            resolve(publicUrl)
          } else if (item.b64_json) {
            resolve('data:image/png;base64,' + item.b64_json)
          } else {
            reject(new Error('返回数据格式异常'))
          }
        } else {
          reject(new Error('请求失败: ' + (res.data && res.data.error ? res.data.error.message : res.statusCode)))
        }
      },
      fail: (err) => reject(new Error('网络请求失败: ' + err.errMsg))
    })
  })
}

/**
 * 将网络图片或临时文件转为 base64
 * @param {string} filePath - 本地临时文件路径
 * @returns {Promise<string>} base64 字符串（不含前缀）
 */
function fileToBase64(filePath) {
  return new Promise((resolve, reject) => {
    wx.getFileSystemManager().readFile({
      filePath: filePath,
      encoding: 'base64',
      success: (res) => resolve(res.data),
      fail: (err) => reject(new Error('读取文件失败: ' + err.errMsg))
    })
  })
}

/**
 * 下载网络图片到本地临时文件
 * @param {string} url - 图片 URL
 * @returns {Promise<string>} 本地临时文件路径
 */
function downloadImage(url) {
  return new Promise((resolve, reject) => {
    wx.downloadFile({
      url: url,
      success: (res) => {
        if (res.statusCode === 200) {
          resolve(res.tempFilePath)
        } else {
          reject(new Error('下载失败'))
        }
      },
      fail: (err) => reject(new Error('下载失败: ' + err.errMsg))
    })
  })
}

module.exports = {
  textToImage,
  imageToImage,
  fileToBase64,
  downloadImage
}
