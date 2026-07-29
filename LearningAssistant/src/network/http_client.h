/*
 * http_client.h —— HTTP 客户端封装
 *
 * 知识点：
 *   1. libcurl easy interface —— 同步 HTTP 请求
 *   2. RAII 封装 curl 句柄
 *   3. 回调函数处理响应数据
 *   4. 设置请求头、超时等选项
 *   5. 条件编译：无 libcurl 时使用 WinHTTP 回退
 */
#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>

namespace la {

/// @brief HTTP 响应
struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    std::string error;

    bool isOk() const { return statusCode >= 200 && statusCode < 300; }
};

/// @brief HTTP 客户端（基于 libcurl 或 WinHTTP回退）
class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    // 禁止拷贝
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    /// 设置默认请求头
    void setDefaultHeader(const std::string& key, const std::string& value);
    /// 设置超时（秒）
    void setTimeout(long seconds);

    // ---- HTTP 方法 ----
    HttpResponse get(const std::string& url);
    HttpResponse post(const std::string& url, const std::string& body = "",
                      const std::string& contentType = "application/json");
    HttpResponse put(const std::string& url, const std::string& body = "");
    HttpResponse del(const std::string& url);

    /// 下载文件
    bool downloadFile(const std::string& url, const std::string& filepath);

private:
    // libcurl 写回调：将响应数据追加到 string
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    static size_t headerCallback(void* contents, size_t size, size_t nmemb, void* userp);

    void* m_curl = nullptr;  // CURL* 句柄（void* 避免头文件依赖）
    std::map<std::string, std::string> m_defaultHeaders;
    long m_timeout = 30;
};

} // namespace la
