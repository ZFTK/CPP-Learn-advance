/*
 * image_processor.cpp —— 图像预处理实现
 */
#ifdef HAS_OPENCV

#include "ocr/image_processor.h"

namespace la {

cv::Mat ImageProcessor::toGrayscale(const cv::Mat& input) {
    cv::Mat gray;
    if (input.channels() == 3) {
        // cv::cvtColor: 颜色空间转换
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = input.clone();
    }
    return gray;
}

cv::Mat ImageProcessor::denoise(const cv::Mat& input) {
    cv::Mat result;
    // cv::GaussianBlur: 高斯模糊去噪
    cv::GaussianBlur(input, result, cv::Size(3, 3), 0);
    // cv::medianBlur: 中值滤波（对椒盐噪声效果好）
    cv::medianBlur(result, result, 3);
    return result;
}

cv::Mat ImageProcessor::adaptiveThreshold(const cv::Mat& gray) {
    cv::Mat binary;
    // cv::adaptiveThreshold: 自适应阈值
    // 每个像素的阈值由其邻域决定，适合光照不均匀的图像
    cv::adaptiveThreshold(gray, binary, 255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY, 11, 2);
    return binary;
}

cv::Mat ImageProcessor::morphology(const cv::Mat& binary, int kernelSize) {
    cv::Mat result;
    // cv::getStructuringElement: 创建形态学操作的结构元素
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT,
        cv::Size(kernelSize, kernelSize));
    // cv::morphologyEx: 形态学操作
    // MORPH_CLOSE: 先膨胀后腐蚀，填充小孔
    cv::morphologyEx(binary, result, cv::MORPH_CLOSE, kernel);
    return result;
}

cv::Mat ImageProcessor::preprocessForOCR(const cv::Mat& input) {
    // OCR 预处理管道：灰度 → 降噪 → 二值化 → 形态学
    cv::Mat gray = toGrayscale(input);
    cv::Mat denoised = denoise(gray);
    cv::Mat binary = adaptiveThreshold(denoised);
    cv::Mat morphed = morphology(binary, 2);
    return morphed;
}

std::vector<cv::Rect> ImageProcessor::detectTextRegions(const cv::Mat& binary) {
    std::vector<cv::Rect> textRegions;

    // cv::findContours: 查找轮廓
    // RETR_EXTERNAL: 只检测外轮廓
    // CHAIN_APPROX_SIMPLE: 压缩水平/垂直/对角线段
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        // 过滤太小的区域（可能是噪声）
        if (rect.width > 20 && rect.height > 10) {
            textRegions.push_back(rect);
        }
    }
    return textRegions;
}

cv::Mat ImageProcessor::drawTextRegions(const cv::Mat& original,
                                         const std::vector<cv::Rect>& regions) {
    cv::Mat result = original.clone();
    for (const auto& rect : regions) {
        // cv::rectangle: 绘制矩形
        cv::rectangle(result, rect, cv::Scalar(0, 255, 0), 2);
    }
    return result;
}

cv::Mat ImageProcessor::resize(const cv::Mat& input, int maxWidth, int maxHeight) {
    double scaleX = static_cast<double>(maxWidth) / input.cols;
    double scaleY = static_cast<double>(maxHeight) / input.rows;
    double scale = std::min(scaleX, scaleY);
    if (scale >= 1.0) return input.clone();

    cv::Mat result;
    // cv::resize: 调整图像大小，INTER_AREA 适合缩小
    cv::resize(input, result, cv::Size(), scale, scale, cv::INTER_AREA);
    return result;
}

cv::Mat ImageProcessor::crop(const cv::Mat& input, const cv::Rect& roi) {
    // Mat 的 ROI 操作：直接使用 () 运算符
    return input(roi).clone();
}

} // namespace la

#endif // HAS_OPENCV
