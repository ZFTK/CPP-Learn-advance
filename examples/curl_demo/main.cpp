/*
 * curl_demo.cpp —— libcurl HTTP 客户端示例
 *
 * 涵盖: GET/POST/PUT/DELETE, Headers, 超时, 下载文件, 多线程并发
 *
 * 编译: g++ -std=c++17 curl_demo.cpp -o curl_demo -lcurl
 *
 * 安装 libcurl:
 *   Windows: vcpkg install curl 或下载预编译包
 *   Linux:   sudo apt install libcurl4-openssl-dev
 */
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>

// 如果安装了 libcurl
#ifdef HAS_CURL
#include <curl/curl.h>

// ==================== RAII 封装 ====================

/// 自动管理 curl 句柄生命周期
class CurlEasy {
public:
    CurlEasy() {
        handle = curl_easy_init();
        if (handle) {
            // 默认设置
            curl_easy_setopt(handle, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
        }
    }

    ~CurlEasy() { if (handle) curl_easy_cleanup(handle); }

    CurlEasy(const CurlEasy&) = delete;
    CurlEasy& operator=(const CurlEasy&) = delete;

    CURL* get() { return handle; }
    operator bool() const { return handle != nullptr; }

    // 设置URL
    void setUrl(const std::string& url) {
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    }

    // 设置请求头
    void setHeaders(const std::vector<std::string>& headers) {
        if (headerList) curl_slist_free_all(headerList);
        headerList = nullptr;
        for (const auto& h : headers) {
            headerList = curl_slist_append(headerList, h.c_str());
        }
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headerList);
    }

    // 执行请求
    std::string perform() {
        responseBody.clear();
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &responseBody);
        CURLcode res = curl_easy_perform(handle);
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
            return "";
        }
        return responseBody;
    }

    // 获取 HTTP 状态码
    long getStatusCode() {
        long code = 0;
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
        return code;
    }

private:
    CURL* handle = nullptr;
    std::string responseBody;
    struct curl_slist* headerList = nullptr;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        ((std::string*)userp)->append((char*)contents, total);
        return total;
    }
};

// ==================== 演示函数 ====================

void demo_get(CurlEasy& curl) {
    std::cout << "=== GET 请求 ===\n";
    curl.setUrl("https://httpbin.org/get");
    curl.setHeaders({"Accept: application/json"});

    std::string response = curl.perform();
    long code = curl.getStatusCode();
    std::cout << "状态码: " << code << "\n";
    std::cout << "响应(前300字符):\n" << response.substr(0, 300) << "\n";
    std::cout << "---\n";
}

void demo_post(CurlEasy& curl) {
    std::cout << "=== POST 请求 ===\n";
    curl.setUrl("https://httpbin.org/post");

    std::string jsonBody = R"({"name":"C++学习助手","action":"test"})";

    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, jsonBody.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, (long)jsonBody.size());
    curl.setHeaders({"Content-Type: application/json"});

    std::string response = curl.perform();
    std::cout << "状态码: " << curl.getStatusCode() << "\n";
    std::cout << "响应(前200字符):\n" << response.substr(0, 200) << "\n";
    std::cout << "---\n";
}

void demo_download(CurlEasy& curl) {
    std::cout << "=== 文件下载 ===\n";
    // 简化演示：写入文件
    curl.setUrl("https://httpbin.org/image/png");
    FILE* fp = fopen("demo_download.png", "wb");
    if (fp) {
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, fp);
        curl.perform();
        fclose(fp);
        std::cout << "已下载到 demo_download.png (状态码: " << curl.getStatusCode() << ")\n";
    }
    std::cout << "---\n";
}

// ==================== 多线程并发下载 ====================
void downloadTask(const std::string& url, int id) {
    CurlEasy curl;
    curl.setUrl(url);
    std::cout << "线程" << id << " 开始下载: " << url.substr(0, 30) << "...\n";
    curl.perform();
    std::cout << "线程" << id << " 完成 (状态码: " << curl.getStatusCode() << ")\n";
}

void demo_multi_thread() {
    std::cout << "=== 多线程并发下载 ===\n";
    std::vector<std::string> urls = {
        "https://httpbin.org/get?page=1",
        "https://httpbin.org/get?page=2",
        "https://httpbin.org/get?page=3",
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < urls.size(); ++i) {
        threads.emplace_back(downloadTask, urls[i], (int)i);
    }
    for (auto& t : threads) t.join();
    std::cout << "全部下载完成\n";
    std::cout << "---\n";
}

int main() {
    // 全局初始化（只需调用一次）
    curl_global_init(CURL_GLOBAL_DEFAULT);

    {
        CurlEasy curl;
        if (!curl) {
            std::cerr << "curl_easy_init 失败\n";
            return 1;
        }

        demo_get(curl);

        // 重置POST设置
        curl_easy_setopt(curl.get(), CURLOPT_POST, 0L);
        curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);

        demo_post(curl);
        demo_download(curl);
    }

    demo_multi_thread();

    curl_global_cleanup();
    std::cout << "=== Curl Demo Complete ===\n";
    return 0;
}

#else
int main() {
    std::cout << "=== Curl 演示 ===\n";
    std::cout << "libcurl 未安装。\n";
    std::cout << "安装方法:\n";
    std::cout << "  Windows: vcpkg install curl\n";
    std::cout << "  Linux:   sudo apt install libcurl4-openssl-dev\n";
    std::cout << "编译:     g++ -std=c++17 curl_demo.cpp -o curl_demo -lcurl -DHAS_CURL\n";
    return 0;
}
#endif
