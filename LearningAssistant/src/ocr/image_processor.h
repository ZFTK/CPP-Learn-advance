/*
 * image_processor.h —— 图像处理与 OCR
 *
 * 知识点：
 *   1. OpenCV 常用预处理：灰度化、二值化、降噪、形态学
 *   2. 轮廓检测 —— cv::findContours
 *   3. 文字区域检测 —— MSER / EAST
 *   4. Tesseract OCR 集成（注释说明）
 *
 * 注意：完整 OCR 需要安装 Tesseract + leptonica
 * 本模块展示 OpenCV 预处理流程，OCR 部分以注释形式说明
 */
#ifdef HAS_OPENCV

#pragma once

#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace la {

/// @brief 检测到的文字区域
struct TextRegion {
    cv::Rect boundingBox;
    std::string text;       // OCR 识别结果（需 Tesseract）
    float confidence = 0.0f;
};

/// @brief 图像处理工具
class ImageProcessor {
public:
    /// 预处理图像以供 OCR 使用
    /// 步骤：灰度化 → 高斯模糊去噪 → 自适应阈值二值化 → 形态学操作
    static cv::Mat preprocessForOCR(const cv::Mat& input);

    /// 灰度化
    static cv::Mat toGrayscale(const cv::Mat& input);

    /// 自适应阈值二值化
    static cv::Mat adaptiveThreshold(const cv::Mat& gray);

    /// 降噪（高斯模糊 + 中值滤波）
    static cv::Mat denoise(const cv::Mat& input);

    /// 形态学操作（膨胀/腐蚀）
    static cv::Mat morphology(const cv::Mat& binary, int kernelSize = 2);

    /// 检测文字区域（基于轮廓）
    static std::vector<cv::Rect> detectTextRegions(const cv::Mat& binary);

    /// 绘制检测到的文字区域
    static cv::Mat drawTextRegions(const cv::Mat& original,
                                   const std::vector<cv::Rect>& regions);

    /// 调整图像大小
    static cv::Mat resize(const cv::Mat& input, int maxWidth, int maxHeight);

    /// 裁剪 ROI 区域
    static cv::Mat crop(const cv::Mat& input, const cv::Rect& roi);
};

/*
 * ===== Tesseract OCR 集成步骤 =====
 *
 * 1. 安装 Tesseract:
 *    Windows: https://github.com/UB-Mannheim/tesseract/wiki
 *    Linux:   sudo apt install tesseract-ocr libtesseract-dev
 *
 * 2. CMakeLists.txt 添加:
 *    find_package(Tesseract REQUIRED)
 *    target_link_libraries(la_ocr PUBLIC Tesseract::libtesseract)
 *
 * 3. 代码示例:
 *    #include <tesseract/baseapi.h>
 *    #include <leptonica/allheaders.h>
 *
 *    tesseract::TessBaseAPI tess;
 *    tess.Init(nullptr, "chi_sim+eng");  // 中英文混合
 *    tess.SetImage(image.data, image.cols, image.rows, 3, image.step);
 *    char* text = tess.GetUTF8Text();
 *    std::string result(text);
 *    delete[] text;
 *    tess.End();
 */

} // namespace la

#endif // HAS_OPENCV
