/*
 * onnx_inference.h —— ONNX Runtime 推理引擎封装
 *
 * 知识点：
 *   1. ONNX Runtime C++ API —— 跨平台 AI 推理
 *   2. 模型加载 (Ort::Session)
 *   3. 输入张量构造 (Ort::Value)
 *   4. 输出张量提取
 *   5. GPU 加速 (CUDA/DirectML)
 *
 * 安装 ONNX Runtime:
 *   https://github.com/microsoft/onnxruntime/releases
 */
#ifdef HAS_ONNX

#pragma once

#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <memory>

namespace la {

/// 张量数据
struct TensorData {
    std::vector<int64_t> shape;
    std::vector<float> data;
    std::string name;
    size_t elementCount() const {
        size_t count = 1;
        for (auto dim : shape) count *= dim;
        return count;
    }
};

/// 推理结果
struct InferenceResult {
    std::vector<TensorData> outputs;
    int64_t inferenceTimeMs = 0;
    bool success = false;
    std::string error;
};

/// ONNX Runtime 推理引擎
class ONNXInference {
public:
    ONNXInference();
    ~ONNXInference();

    bool loadModel(const std::string& modelPath, bool useGPU = false);
    bool isLoaded() const;
    std::vector<TensorData> getInputInfo();
    InferenceResult run(const std::vector<TensorData>& inputs);
    static TensorData createTensor(const std::vector<int64_t>& shape,
                                   const std::vector<float>& data,
                                   const std::string& name = "");

private:
    static Ort::Env& getEnv();
    std::unique_ptr<Ort::Session> m_session;
    Ort::AllocatorWithDefaultOptions m_allocator;
    std::vector<std::string> m_inputNames;
    std::vector<std::string> m_outputNames;
};

/*
 * 与 OpenCV 结合做图像推理示例：
 *
 * // 1. 截取屏幕或加载图像
 * cv::Mat image = cv::imread("test.png");
 *
 * // 2. 预处理（调整为模型输入尺寸）
 * cv::Mat resized, floatMat;
 * cv::resize(image, resized, cv::Size(224, 224));
 * resized.convertTo(floatMat, CV_32FC3, 1.0/255.0);
 *
 * // 3. 转换为 NCHW 格式
 * std::vector<float> inputData(1 * 3 * 224 * 224);
 * for (int c = 0; c < 3; ++c)
 *     for (int h = 0; h < 224; ++h)
 *         for (int w = 0; w < 224; ++w)
 *             inputData[c*224*224 + h*224 + w] = floatMat.at<cv::Vec3f>(h,w)[c];
 *
 * // 4. 推理
 * ONNXInference engine;
 * engine.loadModel("model.onnx");
 * auto result = engine.run({ONNXInference::createTensor({1,3,224,224}, inputData)});
 */

} // namespace la

#endif
