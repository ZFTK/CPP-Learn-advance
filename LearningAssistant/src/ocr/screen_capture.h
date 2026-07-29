/*
 * screen_capture.h —— 屏幕截图 (OpenCV)
 *
 * 知识点：
 *   1. OpenCV Mat —— 图像数据容器
 *   2. 屏幕截图 (Windows: BitBlt, Linux: X11)
 *   3. cv::imencode / cv::imdecode —— 图像编解码
 *   4. 与 OCR 模块集成
 *
 * 编译条件：cmake .. -DENABLE_OPENCV=ON（需安装 OpenCV）
 */
#ifdef HAS_OPENCV

#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace la {

/// @brief 屏幕截图工具
class ScreenCapture {
public:
    ScreenCapture() = default;

    /// 截取整个屏幕
    cv::Mat captureFullScreen();

    /// 截取指定区域 (x, y, width, height)
    cv::Mat captureRegion(int x, int y, int width, int height);

    /// 截取活动窗口
    cv::Mat captureActiveWindow();

    /// 保存为文件
    bool saveToFile(const cv::Mat& image, const std::string& filepath);

    /// 将 Mat 编码为 PNG 字节流（用于网络传输）
    std::vector<unsigned char> encodeToPng(const cv::Mat& image);

    /// 从字节流解码为 Mat
    cv::Mat decodeFromBytes(const std::vector<unsigned char>& data);

private:
#ifdef _WIN32
    cv::Mat captureViaWinAPI(int x, int y, int w, int h);
#endif
};

} // namespace la

#endif // HAS_OPENCV
