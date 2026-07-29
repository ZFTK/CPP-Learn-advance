/*
 * onnx_demo.cpp —— ONNX Runtime 推理速成示例
 *
 * 涵盖: 加载模型、准备输入、执行推理、提取输出
 *
 * 编译: (需要 ONNX Runtime SDK)
 *   g++ -std=c++17 onnx_demo.cpp -o onnx_demo \
 *       -I/path/to/onnxruntime/include \
 *       -L/path/to/onnxruntime/lib -lonnxruntime
 *
 * 安装: https://github.com/microsoft/onnxruntime/releases
 *   下载对应平台的压缩包
 *
 * 典型工作流:
 *   1. Python (PyTorch) 训练模型 → model.pt
 *   2. 导出 ONNX: torch.onnx.export(model, dummy_input, "model.onnx")
 *   3. C++ 加载 model.onnx 进行推理
 */
#ifdef HAS_ONNX

#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <vector>
#include <chrono>

int main() {
    std::cout << "=== ONNX Runtime 推理演示 ===\n";

    // 1. 创建 ONNX Runtime 环境
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ONNX_Demo");

    // 2. 设置会话选项
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(4);               // 线程数
    sessionOptions.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_ENABLE_ALL);           // 图优化

    try {
        // 3. 加载模型
        Ort::Session session(env, L"model.onnx", sessionOptions);

        // 4. 获取模型输入信息
        Ort::AllocatorWithDefaultOptions allocator;
        size_t numInputs = session.GetInputCount();
        std::cout << "输入数量: " << numInputs << "\n";
        for (size_t i = 0; i < numInputs; ++i) {
            auto name = session.GetInputNameAllocated(i, allocator);
            auto typeInfo = session.GetInputTypeInfo(i);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            std::cout << "  输入" << i << ": " << name.get() << " 形状=[";
            for (size_t j = 0; j < shape.size(); ++j) {
                if (j > 0) std::cout << ",";
                std::cout << shape[j];
            }
            std::cout << "]\n";
        }

        // 5. 获取输出信息
        size_t numOutputs = session.GetOutputCount();
        std::cout << "输出数量: " << numOutputs << "\n";

        // 6. 准备输入数据（示例：1x3x224x224 的图像）
        std::vector<int64_t> inputShape = {1, 3, 224, 224};
        size_t inputSize = 1 * 3 * 224 * 224;
        std::vector<float> inputData(inputSize, 0.5f);  // 填充示例数据

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);

        // 创建输入张量
        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(Ort::Value::CreateTensor<float>(
            memoryInfo,
            inputData.data(),
            inputSize,
            inputShape.data(),
            inputShape.size()));

        // 7. 执行推理
        std::vector<const char*> inputNames = {"input"};   // 与模型匹配
        std::vector<const char*> outputNames = {"output"};

        auto start = std::chrono::high_resolution_clock::now();

        auto outputTensors = session.Run(
            Ort::RunOptions{nullptr},
            inputNames.data(), inputTensors.data(), 1,
            outputNames.data(), 1);

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // 8. 提取输出
        auto& output = outputTensors[0];
        auto outputInfo = output.GetTensorTypeAndShapeInfo();
        auto outputShape = outputInfo.GetShape();
        size_t outputCount = outputInfo.GetElementCount();

        float* outputData = output.GetTensorMutableData<float>();

        std::cout << "\n推理完成！耗时: " << duration.count() << "ms\n";
        std::cout << "输出形状: [";
        for (size_t i = 0; i < outputShape.size(); ++i) {
            if (i > 0) std::cout << ",";
            std::cout << outputShape[i];
        }
        std::cout << "]\n";
        std::cout << "输出元素数: " << outputCount << "\n";
        std::cout << "前5个输出值: ";
        for (size_t i = 0; i < std::min<size_t>(5, outputCount); ++i) {
            std::cout << outputData[i] << " ";
        }
        std::cout << "\n";

    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime 错误: " << e.what() << "\n";
        std::cerr << "请确保 model.onnx 存在且格式正确\n";
        return 1;
    }

    std::cout << "\n=== ONNX Demo Complete ===\n";
    return 0;
}

#else
#include <iostream>
int main() {
    std::cout << "=== ONNX Runtime 演示 ===\n";
    std::cout << "ONNX Runtime 未安装。\n\n";
    std::cout << "安装步骤:\n";
    std::cout << "  1. 下载: https://github.com/microsoft/onnxruntime/releases\n";
    std::cout << "  2. 解压到本地目录\n";
    std::cout << "  3. 编译时指定头文件和库路径\n\n";
    std::cout << "Python 导出 ONNX 模型示例:\n";
    std::cout << "  import torch\n";
    std::cout << "  model = YourModel()\n";
    std::cout << "  dummy = torch.randn(1, 3, 224, 224)\n";
    std::cout << "  torch.onnx.export(model, dummy, 'model.onnx')\n";
    return 0;
}
#endif
