/*
 * opencv_demo.cpp —— OpenCV 图像处理速成示例
 *
 * 涵盖: 图像读写、灰度化、模糊、边缘检测、轮廓检测、截图
 *
 * 编译: g++ -std=c++17 opencv_demo.cpp -o opencv_demo -lopencv_core -lopencv_imgcodecs -lopencv_imgproc -lopencv_highgui
 *
 * 安装: https://opencv.org/releases/
 *   Windows: vcpkg install opencv  或下载预编译包
 *   Linux:   sudo apt install libopencv-dev
 */
#ifdef HAS_OPENCV

#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    std::cout << "=== OpenCV 图像处理演示 ===\n";
    std::cout << "OpenCV 版本: " << CV_VERSION << "\n";

    // 1. 创建图像
    std::cout << "\n--- 1. 创建图像 ---\n";
    // Mat: 图像数据容器 (rows=高度, cols=宽度, type=CV_8UC3=8位3通道彩色)
    cv::Mat image(300, 400, CV_8UC3, cv::Scalar(200, 180, 150));
    // 绘制一些图形
    cv::circle(image, cv::Point(200, 150), 80, cv::Scalar(0, 0, 255), 3);  // 红色圆
    cv::rectangle(image, cv::Point(50, 50), cv::Point(150, 100), cv::Scalar(0, 255, 0), 2);  // 绿色矩形
    cv::putText(image, "OpenCV Demo", cv::Point(180, 280),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    cv::imwrite("demo_created.png", image);
    std::cout << "已保存: demo_created.png\n";

    // 2. 图像预处理
    std::cout << "\n--- 2. 图像预处理 ---\n";
    cv::Mat gray;
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);  // 灰度化
    cv::imwrite("demo_gray.png", gray);

    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);  // 高斯模糊去噪
    cv::imwrite("demo_blurred.png", blurred);

    cv::Mat edges;
    cv::Canny(blurred, edges, 50, 150);  // Canny边缘检测
    cv::imwrite("demo_edges.png", edges);
    std::cout << "已保存: demo_gray.png, demo_blurred.png, demo_edges.png\n";

    // 3. 轮廓检测
    std::cout << "\n--- 3. 轮廓检测 ---\n";
    cv::Mat binary;
    cv::threshold(gray, binary, 100, 255, cv::THRESH_BINARY);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    std::cout << "检测到 " << contours.size() << " 个轮廓\n";

    cv::Mat contourImage = image.clone();
    cv::drawContours(contourImage, contours, -1, cv::Scalar(0, 255, 0), 2);
    cv::imwrite("demo_contours.png", contourImage);
    std::cout << "已保存: demo_contours.png\n";

    // 4. 图像读写
    std::cout << "\n--- 4. 图像属性 ---\n";
    cv::Mat loaded = cv::imread("demo_created.png");
    if (!loaded.empty()) {
        std::cout << "尺寸: " << loaded.cols << "x" << loaded.rows << "\n";
        std::cout << "通道数: " << loaded.channels() << "\n";
        std::cout << "类型: " << loaded.type() << "\n";
        // 访问像素
        cv::Vec3b pixel = loaded.at<cv::Vec3b>(100, 100);
        std::cout << "像素(100,100): B=" << (int)pixel[0]
                  << " G=" << (int)pixel[1] << " R=" << (int)pixel[2] << "\n";
    }

    // 5. 图像变换
    std::cout << "\n--- 5. 图像变换 ---\n";
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(200, 150));  // 缩放
    cv::imwrite("demo_resized.png", resized);

    cv::Mat rotated;
    cv::Mat M = cv::getRotationMatrix2D(cv::Point(200, 150), 45, 1.0);
    cv::warpAffine(image, rotated, M, image.size());  // 旋转
    cv::imwrite("demo_rotated.png", rotated);
    std::cout << "已保存: demo_resized.png, demo_rotated.png\n";

    std::cout << "\n=== OpenCV Demo Complete ===\n";
    return 0;
}

#else
#include <iostream>
int main() {
    std::cout << "=== OpenCV 演示 ===\n";
    std::cout << "OpenCV 未安装。安装方法:\n";
    std::cout << "  Windows: vcpkg install opencv\n";
    std::cout << "  Linux:   sudo apt install libopencv-dev\n";
    std::cout << "  macOS:   brew install opencv\n";
    return 0;
}
#endif
