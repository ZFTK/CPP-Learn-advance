# 10 — OpenCV 图像处理实战

## 1. 核心概念

OpenCV（Open Source Computer Vision Library）是计算机视觉领域的行业标准库，提供2500+优化算法，覆盖图像处理、特征检测、目标识别、视频分析等。

核心数据结构：
- `cv::Mat` — 多维矩阵，所有图像的基础容器（头 + 数据指针，引用计数管理）
- `cv::Size` / `cv::Point` / `cv::Rect` / `cv::Scalar` — 几何与颜色辅助类型

**为什么用OpenCV？**一体化的图像IO、变换、滤波、绘图、视频、GUI，行业标准，社区庞大。

---

## 2. 快速上手

### CMake 引入

```cmake
find_package(OpenCV REQUIRED)
target_link_libraries(myapp PRIVATE ${OpenCV_LIBS})
# 或更精确：
# find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
# target_link_libraries(myapp PRIVATE opencv_core opencv_imgproc ...)
```

### 最小示例

```cpp
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 读取图像
    cv::Mat img = cv::imread("input.jpg", cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "无法读取图像\n";
        return 1;
    }

    std::cout << "尺寸: " << img.cols << " x " << img.rows
              << ", 通道: " << img.channels() << "\n";

    // 转换为灰度
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // 保存
    cv::imwrite("output_gray.jpg", gray);

    // 显示（需要GUI支持）
    cv::imshow("原图", img);
    cv::imshow("灰度", gray);
    cv::waitKey(0);  // 等待按键

    return 0;
}
```

---

## 3. 常用API速查

### 3.1 Mat 基础操作

```cpp
// 创建
cv::Mat m1(480, 640, CV_8UC3);                  // 480行640列，8位3通道
cv::Mat m2(480, 640, CV_8UC3, cv::Scalar(0,255,0)); // 全绿色
cv::Mat m3 = cv::Mat::zeros(100, 200, CV_32FC1); // 全0单精度浮点
cv::Mat m4 = cv::Mat::ones(100, 200, CV_8UC1);   // 全1

// 类型码解读：CV_8UC3 = 8位无符号，3通道
// CV_8UC1=灰度, CV_8UC3=BGR彩色, CV_32FC1=float单通道

// 访问像素
// 方式1：at（安全，有类型检查）
cv::Vec3b& pixel = img.at<cv::Vec3b>(row, col);   // BGR
pixel[0] = 255;  // B
pixel[1] = 0;    // G
pixel[2] = 0;    // R

// 灰度图
uchar& gray_pixel = gray.at<uchar>(row, col);

// 方式2：ptr（高效逐行访问）
for (int r = 0; r < img.rows; ++r) {
    cv::Vec3b* row_ptr = img.ptr<cv::Vec3b>(r);
    for (int c = 0; c < img.cols; ++c) {
        row_ptr[c] = cv::Vec3b(0, 0, 255); // 全红
    }
}

// 方式3：data指针（最快，需手动计算偏移）
uchar* p = img.data;
p[row * img.step + col * img.channels() + channel] = value;
```

### 3.2 基本变换

```cpp
#include <opencv2/imgproc.hpp>

cv::Mat src = cv::imread("input.jpg");
cv::Mat dst;

// 缩放
cv::resize(src, dst, cv::Size(320, 240));
cv::resize(src, dst, cv::Size(), 0.5, 0.5);  // 缩放50%

// 旋转
cv::Mat rot = cv::getRotationMatrix2D(cv::Point2f(src.cols/2, src.rows/2), 45, 1.0);
cv::warpAffine(src, dst, rot, src.size());

// 裁剪（ROI——零拷贝，仅修改头）
cv::Rect roi(100, 50, 200, 200);            // x, y, width, height
cv::Mat crop = src(roi);                     // 与原图共享数据！
cv::Mat crop_copy = src(roi).clone();        // 深拷贝

// 翻转
cv::flip(src, dst, 0);   // 上下
cv::flip(src, dst, 1);   // 左右
cv::flip(src, dst, -1);  // 上下+左右

// 颜色空间转换
cv::cvtColor(src, dst, cv::COLOR_BGR2GRAY);
cv::cvtColor(src, dst, cv::COLOR_BGR2HSV);
cv::cvtColor(dst, dst, cv::COLOR_HSV2BGR);  // 还原

// 滤波
cv::GaussianBlur(src, dst, cv::Size(5, 5), 0);  // 高斯模糊
cv::medianBlur(src, dst, 5);                      // 中值滤波
cv::Canny(src, dst, 50, 150);                     // 边缘检测

// 阈值
cv::threshold(gray, dst, 128, 255, cv::THRESH_BINARY);
cv::adaptiveThreshold(gray, dst, 255,
    cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

// 形态学
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
cv::erode(src, dst, kernel);   // 腐蚀
cv::dilate(src, dst, kernel);  // 膨胀
```

### 3.3 绘图

```cpp
// 在图像上绘图（直接修改原图）
cv::Mat canvas(400, 600, CV_8UC3, cv::Scalar(255, 255, 255));

// 线段
cv::line(canvas, cv::Point(50, 50), cv::Point(200, 200),
         cv::Scalar(0, 0, 255), 2);  // 红色，线宽2

// 矩形
cv::rectangle(canvas, cv::Rect(100, 100, 150, 100),
              cv::Scalar(0, 255, 0), 2);  // 绿色边框
cv::rectangle(canvas, cv::Rect(300, 100, 150, 100),
              cv::Scalar(255, 0, 0), cv::FILLED); // 蓝色填充

// 圆形
cv::circle(canvas, cv::Point(400, 300), 50,
           cv::Scalar(0, 255, 255), -1);  // 黄色填充

// 文字
cv::putText(canvas, "Hello OpenCV", cv::Point(50, 350),
            cv::FONT_HERSHEY_SIMPLEX, 1.0,
            cv::Scalar(0, 0, 0), 2);

// 保存
cv::imwrite("drawing.png", canvas);
```

### 3.4 截图功能

```cpp
#ifdef _WIN32
#include <windows.h>

cv::Mat capture_screen(int x, int y, int width, int height) {
    // 获取屏幕DC
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hBitmap);

    // 拷贝屏幕像素
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, SRCCOPY);

    // 转 OpenCV Mat
    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), width, -height, 1, 32, BI_RGB };
    cv::Mat mat(height, width, CV_8UC4);
    GetDIBits(hdcMem, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);

    // BGRA → BGR
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}
#endif
```

---

## 4. Tesseract OCR 集成思路

```cpp
// 需要安装 Tesseract 和 Leptonica
// find_package(Tesseract REQUIRED)
// target_link_libraries(myapp PRIVATE ${Tesseract_LIBRARIES})

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

std::string ocr_image(const cv::Mat& img) {
    // 转灰度
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
    // 二值化提升识别率
    cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    // 初始化Tesseract
    tesseract::TessBaseAPI tess;
    if (tess.Init(nullptr, "chi_sim+eng")) { // 中英文
        return "Tesseract初始化失败";
    }

    tess.SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);
    char* text = tess.GetUTF8Text();

    std::string result(text);
    delete[] text;
    tess.End();

    return result;
}
```

---

## 5. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **ROI浅拷贝** | `Mat crop = img(rect)` 共享数据，修改crop会影响原图 | 需要独立数据时加 `.clone()` |
| **BGR而非RGB** | OpenCV默认BGR顺序 | 与外部库互转时用 `cv::cvtColor(img, img, cv::COLOR_BGR2RGB)` |
| **类型不匹配** | `at<Vec3b>` 用在灰度图上会崩溃 | 确认 `img.type()` 再访问 |
| **路径中文乱码** | Windows下 `imread` 不支持中文路径 | 用 `imdecode` + 文件读取 |
| **未检查 empty()** | 读取失败后继续操作 | 所有 `imread` 后检查 `img.empty()` |
| **内存泄漏** | `cv::Mat` 引用计数管理，一般不会，但调第三方API时注意 | 使用 `Mat` 原始数据指针时确保 `Mat` 对象存活 |
| **waitKey(0) 无效** | 无GUI编译（headless）时 `imshow`/`waitKey` 不可用 | 服务端用 `imwrite` 输出，不做 `imshow` |

**最佳实践：**

- 读取图像后始终检查 `img.empty()`
- 大量处理时用 `ptr<>` 逐行访问而非 `at<>`
- 颜色空间转换方向：`COLOR_BGR2GRAY` / `COLOR_BGR2RGB`
- Windows 中文路径用 `imdecode` 解决：

```cpp
// 解决中文路径
cv::Mat imread_zh(const std::string& path) {
    std::ifstream fs(path, std::ios::binary);
    std::vector<uchar> buf((std::istreambuf_iterator<char>(fs)),
                            std::istreambuf_iterator<char>());
    return cv::imdecode(buf, cv::IMREAD_COLOR);
}
```

---

## 参考链接

- [OpenCV 官方文档](https://docs.opencv.org/4.x/)
- [Tesseract OCR](https://github.com/tesseract-ocr/tesseract)
