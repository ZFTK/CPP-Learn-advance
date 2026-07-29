/*
 * onnx_inference.cpp —— ONNX Runtime 推理实现
 */
#ifdef HAS_ONNX

#include "ai/onnx_inference.h"
#include "core/logger.h"
#include <chrono>

namespace la {

Ort::Env& ONNXInference::getEnv() {
    // 单例 Ort::Env —— 线程安全，全局只需一个
    static Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "LearningAssistant");
    return env;
}

ONNXInference::ONNXInference() {}
ONNXInference::~ONNXInference() {}

bool ONNXInference::loadModel(const std::string& modelPath, bool useGPU) {
    try {
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(4);     // 线程数
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        if (useGPU) {
            // 尝试 CUDA
            try {
                OrtCUDAProviderOptions cudaOpts;
                options.AppendExecutionProvider_CUDA(cudaOpts);
                LA_LOG_INFO("Using CUDA execution provider");
            } catch (...) {
                // 回退到 DirectML (Windows)
                try {
                    options.AppendExecutionProvider_DML(0);
                    LA_LOG_INFO("Using DirectML execution provider");
                } catch (...) {
                    LA_LOG_WARNING("GPU not available, using CPU");
                }
            }
        }

        // 创建 Session —— 加载模型
        m_session = std::make_unique<Ort::Session>(getEnv(),
            std::wstring(modelPath.begin(), modelPath.end()).c_str(),
            options);

        // 获取输入输出名称
        size_t numInputs = m_session->GetInputCount();
        for (size_t i = 0; i < numInputs; ++i) {
            auto name = m_session->GetInputNameAllocated(i, m_allocator);
            m_inputNames.push_back(name.get());
        }

        size_t numOutputs = m_session->GetOutputCount();
        for (size_t i = 0; i < numOutputs; ++i) {
            auto name = m_session->GetOutputNameAllocated(i, m_allocator);
            m_outputNames.push_back(name.get());
        }

        LA_LOG_INFO("Model loaded: " + modelPath +
                    " inputs=" + std::to_string(numInputs) +
                    " outputs=" + std::to_string(numOutputs));
        return true;
    } catch (const Ort::Exception& e) {
        LA_LOG_ERROR("Failed to load model: " + std::string(e.what()));
        return false;
    }
}

bool ONNXInference::isLoaded() const {
    return m_session != nullptr;
}

std::vector<TensorData> ONNXInference::getInputInfo() {
    std::vector<TensorData> result;
    if (!m_session) return result;

    for (size_t i = 0; i < m_session->GetInputCount(); ++i) {
        TensorData info;
        info.name = m_inputNames[i];
        auto typeInfo = m_session->GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        info.shape = tensorInfo.GetShape();
        result.push_back(info);
    }
    return result;
}

InferenceResult ONNXInference::run(const std::vector<TensorData>& inputs) {
    InferenceResult result;
    if (!m_session) {
        result.error = "Model not loaded";
        return result;
    }

    auto start = std::chrono::high_resolution_clock::now();

    try {
        // 准备 Ort::Value 输入张量
        std::vector<Ort::Value> ortInputs;
        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);

        for (const auto& input : inputs) {
            ortInputs.push_back(Ort::Value::CreateTensor<float>(
                memInfo,
                const_cast<float*>(input.data.data()),  // 数据指针
                input.data.size(),                       // 元素数
                input.shape.data(),                      // 形状
                input.shape.size()));                    // 维度数
        }

        // 准备输出名称的 C 字符串数组
        std::vector<const char*> outputNamesRaw;
        for (const auto& name : m_outputNames) outputNamesRaw.push_back(name.c_str());
        std::vector<const char*> inputNamesRaw;
        for (const auto& name : m_inputNames) inputNamesRaw.push_back(name.c_str());

        // 执行推理
        auto ortOutputs = m_session->Run(Ort::RunOptions{nullptr},
            inputNamesRaw.data(), ortInputs.data(), ortInputs.size(),
            outputNamesRaw.data(), outputNamesRaw.size());

        // 提取输出
        for (size_t i = 0; i < ortOutputs.size(); ++i) {
            TensorData output;
            output.name = m_outputNames[i];
            auto tensorInfo = ortOutputs[i].GetTensorTypeAndShapeInfo();
            output.shape = tensorInfo.GetShape();
            size_t count = tensorInfo.GetElementCount();
            output.data.resize(count);
            // 拷贝输出数据
            std::memcpy(output.data.data(),
                ortOutputs[i].GetTensorMutableData<float>(),
                count * sizeof(float));
            result.outputs.push_back(std::move(output));
        }

        result.success = true;
    } catch (const Ort::Exception& e) {
        result.error = e.what();
        LA_LOG_ERROR("Inference failed: " + result.error);
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.inferenceTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

TensorData ONNXInference::createTensor(const std::vector<int64_t>& shape,
                                        const std::vector<float>& data,
                                        const std::string& name) {
    TensorData tensor;
    tensor.shape = shape;
    tensor.data = data;
    tensor.name = name.empty() ? "input" : name;
    return tensor;
}

} // namespace la

#endif // HAS_ONNX
