# 11 — ONNX Runtime C++ 推理指南

## 1. 核心概念

ONNX（Open Neural Network Exchange）是开放的深度学习模型交换格式，由微软和Meta联合发起。ONNX Runtime 是跨平台高性能推理引擎，支持从 PyTorch / TensorFlow / Keras 等框架导出的模型。

核心流程：**训练框架 → 导出.onnx → ONNX Runtime 加载 → C++ 推理**

**为什么用 ONNX Runtime？**
- 一套C++ API推理所有框架的模型
- GPU/CPU 自动切换，支持 CUDA / DirectML / OpenVINO 等后端
- 工业级优化：图融合、量化、内存规划
- 部署场景首选：桌面应用、边缘设备、服务端

---

## 2. 快速上手

### CMake 引入

```cmake
# 方式一：系统安装（推荐）
find_package(onnxruntime REQUIRED)
target_link_libraries(myapp PRIVATE onnxruntime)

# 方式二：手动指定路径
set(ONNXRUNTIME_ROOT "C:/Program Files/onnxruntime")
target_include_directories(myapp PRIVATE ${ONNXRUNTIME_ROOT}/include)
target_link_directories(myapp PRIVATE ${ONNXRUNTIME_ROOT}/lib)
target_link_libraries(myapp PRIVATE onnxruntime)
```

### 最小示例：运行一个简单模型

```cpp
#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <vector>

int main() {
    // 1. 初始化环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    Ort::SessionOptions session_options;
    session_options.SetIntraOpNumThreads(4);           // 线程数
    // session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

    // 2. 加载模型
    const wchar_t* model_path = L"model.onnx";         // Windows用宽字符
    // Linux/Mac: const char* model_path = "model.onnx";
    Ort::Session session(env, model_path, session_options);

    // 3. 获取输入输出信息
    Ort::AllocatorWithDefaultOptions allocator;

    size_t num_inputs = session.GetInputCount();
    for (size_t i = 0; i < num_inputs; ++i) {
        auto name = session.GetInputNameAllocated(i, allocator);
        auto type_info = session.GetInputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        auto shape = tensor_info.GetShape();
        std::cout << "输入 " << i << ": " << name.get()
                  << ", 类型: " << tensor_info.GetElementType() << "\n";
    }

    size_t num_outputs = session.GetOutputCount();
    for (size_t i = 0; i < num_outputs; ++i) {
        auto name = session.GetOutputNameAllocated(i, allocator);
        std::cout << "输出 " << i << ": " << name.get() << "\n";
    }

    std::cout << "模型加载成功!\n";
    return 0;
}
```

---

## 3. 核心API速查

### 3.1 准备输入张量

```cpp
#include <onnxruntime_cxx_api.h>

// 创建内存信息（CPU）
Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(
    OrtArenaAllocator, OrtMemTypeDefault);

// 准备输入数据（假设模型输入: 1x3x224x224, float32）
std::vector<float> input_data(1 * 3 * 224 * 224, 0.5f); // 示例数据
std::vector<int64_t> input_shape = {1, 3, 224, 224};

Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
    memory_info,
    input_data.data(),
    input_data.size(),
    input_shape.data(),
    input_shape.size()
);

// 输入名称
const char* input_names[] = {"input"};      // 需与模型匹配！
const char* output_names[] = {"output"};

// 运行推理
auto outputs = session.Run(
    Ort::RunOptions{nullptr},
    input_names,
    &input_tensor, 1,
    output_names, 1
);
```

### 3.2 获取输出结果

```cpp
// 获取第一个输出
auto& output_tensor = outputs.front();

// 获取输出张量信息
auto output_info = output_tensor.GetTensorTypeAndShapeInfo();
auto output_shape = output_info.GetShape();
size_t output_count = output_info.GetElementCount();

// 获取数据指针
float* output_data = output_tensor.GetTensorMutableData<float>();

// 处理结果（例如分类任务，取最大值索引）
int class_id = 0;
float max_score = output_data[0];
for (size_t i = 1; i < output_count; ++i) {
    if (output_data[i] > max_score) {
        max_score = output_data[i];
        class_id = static_cast<int>(i);
    }
}
std::cout << "预测类别: " << class_id << ", 置信度: " << max_score << "\n";
```

### 3.3 使用 CUDA GPU

```cpp
// 检查CUDA是否可用
std::vector<std::string> providers = Ort::GetAvailableProviders();
bool has_cuda = false;
for (const auto& p : providers) {
    if (p.find("CUDA") != std::string::npos) has_cuda = true;
}

if (has_cuda) {
    Ort::SessionOptions opts;
    // 方法1：通过 OrtCUDAProviderOptions
    OrtCUDAProviderOptions cuda_opts;
    cuda_opts.device_id = 0;
    opts.AppendExecutionProvider_CUDA(cuda_opts);
    // 方法2：简单追加
    // OrtSessionOptionsAppendExecutionProvider_CUDA(opts, 0);

    Ort::Session session(env, model_path, opts);
    std::cout << "使用 CUDA 推理\n";
} else {
    std::cout << "CUDA 不可用，使用 CPU\n";
}
```

### 3.4 动态形状输入

```cpp
// 有些模型输入形状可变（如 batch size 可为 1~N）
Ort::Session session(env, model_path, session_options);

// 获取输入形状（可能含动态维度，如 {-1, 3, 224, 224}）
auto type_info = session.GetInputTypeInfo(0);
auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
auto symbolic_shape = tensor_info.GetShape();

// 指定实际形状
std::vector<int64_t> actual_shape = {2, 3, 224, 224};  // batch=2

std::vector<float> input_data(2 * 3 * 224 * 224);
Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
    memory_info, input_data.data(), input_data.size(),
    actual_shape.data(), actual_shape.size());
```

---

## 4. 与 OpenCV 结合进行推理

### 场景：图像分类完整流程

```cpp
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <iostream>
#include <fstream>

class ImageClassifier {
public:
    ImageClassifier(const std::wstring& model_path, const std::string& label_path,
                    int img_size = 224)
        : img_size_(img_size)
    {
        // 加载标签
        std::ifstream file(label_path);
        std::string line;
        while (std::getline(file, line)) labels_.push_back(line);

        // 创建ONNX Runtime会话
        session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(),
                                                   Ort::SessionOptions{});
    }

    // 预处理：OpenCV Mat → ONNX 输入张量
    std::vector<float> preprocess(const cv::Mat& img) {
        // 缩放
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(img_size_, img_size_));

        // 转换 BGR → RGB 并归一化到 [0, 1]
        cv::Mat rgb;
        cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
        rgb.convertTo(rgb, CV_32FC3, 1.0 / 255.0);

        // 转换为 CHW 格式 (Channel, Height, Width)
        std::vector<float> data(3 * img_size_ * img_size_);
        std::vector<cv::Mat> channels(3);
        cv::split(rgb, channels);  // 分离为 R, G, B 三个单通道

        for (int c = 0; c < 3; ++c) {
            std::memcpy(data.data() + c * img_size_ * img_size_,
                        channels[c].data,
                        img_size_ * img_size_ * sizeof(float));
        }
        return data;
    }

    // 推理
    struct Result {
        int class_id;
        std::string label;
        float confidence;
    };

    Result classify(const cv::Mat& img) {
        auto input_data = preprocess(img);
        std::vector<int64_t> shape = {1, 3, img_size_, img_size_};

        Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input = Ort::Value::CreateTensor<float>(
            mem_info, input_data.data(), input_data.size(),
            shape.data(), shape.size());

        const char* input_names[] = {"input"};
        const char* output_names[] = {"output"};

        auto outputs = session_->Run(Ort::RunOptions{nullptr},
                                     input_names, &input, 1,
                                     output_names, 1);

        float* output_data = outputs.front().GetTensorMutableData<float>();
        size_t count = outputs.front().GetTensorTypeAndShapeInfo().GetElementCount();

        // 找出最大概率
        int best_idx = 0;
        float best_score = output_data[0];
        for (size_t i = 1; i < count; ++i) {
            if (output_data[i] > best_score) {
                best_score = output_data[i];
                best_idx = static_cast<int>(i);
            }
        }

        return {best_idx,
                best_idx < (int)labels_.size() ? labels_[best_idx] : "unknown",
                best_score};
    }

private:
    Ort::Env env_{ORT_LOGGING_LEVEL_WARNING, "ImageClassifier"};
    std::unique_ptr<Ort::Session> session_;
    std::vector<std::string> labels_;
    int img_size_;
};

// 使用
int main() {
    try {
        ImageClassifier classifier(L"resnet18.onnx", "imagenet_labels.txt");

        cv::Mat img = cv::imread("cat.jpg");
        if (img.empty()) {
            std::cerr << "无法读取图像\n";
            return 1;
        }

        auto result = classifier.classify(img);
        std::cout << "预测: " << result.label
                  << " (置信度: " << result.confidence * 100 << "%)\n";

    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX错误: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

---

## 5. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **输入形状不匹配** | 模型期望(1,3,224,224)，传了(224,224,3) | 确认 CHW vs HWC，batch维度 |
| **像素范围错误** | 模型期望[0,1]归一化，传了[0,255] | 查看训练时的预处理方式 |
| **通道顺序** | 模型期望RGB，OpenCV是BGR | 用 `cvtColor` 转为 RGB |
| **onnxruntime dll缺失** | 运行时找不到 `onnxruntime.dll` | 复制到exe目录或添加到PATH |
| **跨平台模型路径** | Windows用 `L"model.onnx"` 宽字符 | 封装平台相关路径处理 |
| **内存拷贝开销** | 每次推理都重新创建 `Ort::Value` | 复用预创建的 `Ort::Value`，仅更新数据 |
| **session线程安全** | 多线程共享同一session需注意 | 每个线程独立session，或用锁保护 |

**最佳实践：**

- 预处理（normalize/mean/std）参考模型训练代码或ONNX模型文档
- 生产环境开启图优化：`session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL)`
- 固定输入形状的模型性能更好（避免动态形状开销）
- 频繁推理时复用 `Ort::Value` 避免重复分配
- 使用 `Ort::MemoryInfo` 指定设备避免CPU-GPU数据拷贝

---

## 6. 典型项目结构

```
inference_app/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── classifier.cpp          # ImageClassifier 实现
│   └── preprocess.cpp          # 图像预处理
├── include/
│   └── classifier.h
├── models/
│   ├── resnet18.onnx
│   └── imagenet_labels.txt
└── images/
    └── test.jpg
```

---

## 参考链接

- [ONNX Runtime 官方文档](https://onnxruntime.ai/docs/)
- [ONNX Model Zoo](https://github.com/onnx/models)
- [OpenCV DNN 模块（内置ONNX支持，轻量替代）](https://docs.opencv.org/4.x/d2/d58/tutorial_table_of_content_dnn.html)
