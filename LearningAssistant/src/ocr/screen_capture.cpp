/*
 * screen_capture.cpp —— 屏幕截图实现
 *
 * 知识点：
 *   1. Windows GDI: GetDC / BitBlt / CreateCompatibleDC
 *   2. OpenCV Mat: 构造、拷贝、ROI
 *   3. cv::imencode: 图像编码压缩
 *   4. cv::cvtColor: 颜色空间转换
 */
#ifdef HAS_OPENCV

#include "ocr/screen_capture.h"
#include "core/logger.h"

#ifdef _WIN32
#include <windows.h>

namespace la {

cv::Mat ScreenCapture::captureViaWinAPI(int x, int y, int w, int h) {
    // 1. 获取屏幕 DC (Device Context)
    HDC hScreenDC = GetDC(nullptr);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // 2. 创建兼容位图
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, w, h);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // 3. 将屏幕内容复制到位图
    // BitBlt: 位块传输，将像素从一个DC复制到另一个DC
    BitBlt(hMemoryDC, 0, 0, w, h, hScreenDC, x, y, SRCCOPY);

    // 4. 获取位图数据
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = w;
    bi.biHeight = -h;  // 负值表示 top-down（原点在左上角）
    bi.biPlanes = 1;
    bi.biBitCount = 32;  // 32位色深（BGRA）
    bi.biCompression = BI_RGB;

    // 5. 创建 OpenCV Mat 并填充数据
    cv::Mat mat(h, w, CV_8UC4);  // 8位无符号，4通道 (BGRA)
    GetDIBits(hMemoryDC, hBitmap, 0, h, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // 6. BGRA → BGR（OpenCV 默认三通道）
    cv::Mat bgr;
    cv::cvtColor(mat, bgr, cv::COLOR_BGRA2BGR);

    // 7. 清理 GDI 资源
    SelectObject(hMemoryDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(nullptr, hScreenDC);

    return bgr;
}

cv::Mat ScreenCapture::captureFullScreen() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    LA_LOG_INFO("Capturing full screen: " + std::to_string(w) + "x" + std::to_string(h));
    return captureViaWinAPI(0, 0, w, h);
}

cv::Mat ScreenCapture::captureRegion(int x, int y, int width, int height) {
    return captureViaWinAPI(x, y, width, height);
}

cv::Mat ScreenCapture::captureActiveWindow() {
    HWND hwnd = GetForegroundWindow();
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int w = rect.right - rect.left;
    int h = rect.bottom - rect.top;
    return captureViaWinAPI(rect.left, rect.top, w, h);
}

#endif // _WIN32

bool ScreenCapture::saveToFile(const cv::Mat& image, const std::string& filepath) {
    // cv::imwrite: 将 Mat 写入文件（自动根据扩展名选择编码）
    return cv::imwrite(filepath, image);
}

std::vector<unsigned char> ScreenCapture::encodeToPng(const cv::Mat& image) {
    std::vector<unsigned char> buffer;
    // cv::imencode: 将 Mat 编码到内存缓冲区
    cv::imencode(".png", image, buffer);
    return buffer;
}

cv::Mat ScreenCapture::decodeFromBytes(const std::vector<unsigned char>& data) {
    // cv::imdecode: 从内存缓冲区解码
    return cv::imdecode(data, cv::IMREAD_COLOR);
}

} // namespace la

#endif // HAS_OPENCV
