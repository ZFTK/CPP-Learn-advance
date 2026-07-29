# 08 — libcurl 网络编程实战

## 1. 核心概念

libcurl 是C/C++最广泛使用的网络传输库，支持 HTTP/HTTPS、FTP、SMTP 等数十种协议。它跨平台、线程安全、支持异步，是 curl 命令行工具的同源库。

**核心模型：** `easy handle`（同步单连接）和 `multi handle`（多路并发）。easy handle 是基础，足够覆盖 90% 的场景。

**为什么用 libcurl？**无需造轮子处理 HTTP/SSL/重定向/认证/cookie/代理，一行 `curl_easy_perform()` 搞定，所有主流平台预装或易装。

---

## 2. 快速上手

### CMake 引入

```cmake
# 系统通常已安装
find_package(CURL REQUIRED)
target_link_libraries(myapp PRIVATE CURL::libcurl)

# 或用 FetchContent
include(FetchContent)
FetchContent_Declare(curl
    URL https://curl.se/download/curl-8.7.1.tar.xz
)
```

### 最小示例：GET 请求

```cpp
#include <curl/curl.h>
#include <iostream>
#include <string>

// 回调函数：libcurl 把响应数据分段传进来
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), total);
    return total;  // 返回处理的字节数
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);  // 全局初始化（程序生命周期一次）

    CURL* curl = curl_easy_init();
    if (!curl) return 1;

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, "https://httpbin.org/get");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);  // 执行请求
    if (res != CURLE_OK) {
        std::cerr << "curl 失败: " << curl_easy_strerror(res) << "\n";
    } else {
        std::cout << response << "\n";
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
```

---

## 3. 常用API / 写法速查

### 3.1 POST 请求

```cpp
CURL* curl = curl_easy_init();
std::string response;

// POST JSON 数据
const char* json_data = R"({"name":"张三","age":28})";

struct curl_slist* headers = nullptr;
headers = curl_slist_append(headers, "Content-Type: application/json");

curl_easy_setopt(curl, CURLOPT_URL, "https://httpbin.org/post");
curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(json_data));
curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

curl_easy_perform(curl);

curl_slist_free_all(headers);
curl_easy_cleanup(curl);
```

### 3.2 设置自定义 Headers

```cpp
struct curl_slist* headers = nullptr;
headers = curl_slist_append(headers, "Authorization: Bearer xxxxxx");
headers = curl_slist_append(headers, "Accept: application/json");
headers = curl_slist_append(headers, "User-Agent: MyApp/1.0");

curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

// 用完后
curl_slist_free_all(headers);
```

### 3.3 常见 curl_easy_setopt 选项

```cpp
// 超时设置
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);           // 总超时30秒
curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);     // 连接超时10秒

// 重定向
curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);      // 跟随重定向
curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);           // 最多5次

// SSL 相关
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);      // 验证服务器证书（生产务必开启）
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);      // 验证hostname
// 自签名证书场景：
// curl_easy_setopt(curl, CURLOPT_CAINFO, "my-ca-bundle.crt");

// 代理
curl_easy_setopt(curl, CURLOPT_PROXY, "http://127.0.0.1:8080");

// Cookie
curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt"); // 从文件读cookie
curl_easy_setopt(curl, CURLOPT_COOKIEJAR,  "cookies.txt"); // 写入cookie到文件

// 详细日志（调试用）
curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

// 获取响应状态码
long http_code = 0;
curl_easy_setopt(curl, CURLOPT_RESPONSE_CODE, &http_code);
// 执行后 curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
```

### 3.4 多线程下载（multi interface）

```cpp
#include <vector>
#include <thread>

// 简单方案：每个线程一个 easy handle
void download_file(const std::string& url, const std::string& filename) {
    CURL* curl = curl_easy_init();
    FILE* fp = fopen(filename.c_str(), "wb");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::vector<std::string> urls = {
        "https://example.com/file1.zip",
        "https://example.com/file2.zip",
        "https://example.com/file3.zip",
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < urls.size(); ++i) {
        threads.emplace_back(download_file, urls[i],
                             "download_" + std::to_string(i) + ".tmp");
    }
    for (auto& t : threads) t.join();

    curl_global_cleanup();
    return 0;
}
```

---

## 4. C++ RAII 封装

```cpp
class CurlEasy {
public:
    CurlEasy() {
        curl_ = curl_easy_init();
        if (!curl_) throw std::runtime_error("curl_easy_init 失败");
    }

    ~CurlEasy() {
        if (curl_) curl_easy_cleanup(curl_);
        if (headers_) curl_slist_free_all(headers_);
    }

    CurlEasy(const CurlEasy&) = delete;
    CurlEasy& operator=(const CurlEasy&) = delete;

    void set_url(const std::string& url) {
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    }

    void set_header(const std::string& header) {
        headers_ = curl_slist_append(headers_, header.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    }

    void set_post_fields(const std::string& data) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, (long)data.size());
    }

    void set_timeout(long seconds) {
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, seconds);
    }

    std::pair<CURLcode, std::string> perform() {
        std::string response;
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl_);
        return {res, std::move(response)};
    }

    long get_response_code() {
        long code = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code);
        return code;
    }

    CURL* handle() { return curl_; }

private:
    CURL* curl_ = nullptr;
    struct curl_slist* headers_ = nullptr;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        static_cast<std::string*>(userp)->append(static_cast<char*>(contents), total);
        return total;
    }
};

// 全局初始化器（RAII）
class CurlGlobal {
public:
    CurlGlobal()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};
```

---

## 5. 常见陷阱 & 最佳实践

| 陷阱 | 说明 | 正确做法 |
|------|------|----------|
| **忘记 curl_global_init** | 多数平台静默工作，但线程/SSL可能异常 | main函数入口调用一次 |
| **回调参数不返回大小** | 回调返回值不等于(size*nmemb)会导致传输中断 | 始终返回 `size * nmemb` |
| **SSL 验证跳过** | `CURLOPT_SSL_VERIFYPEER=0` 生产环境禁用 | 生产环境始终验证SSL |
| **不设置超时** | 网络故障导致永久阻塞 | 设置 `CURLOPT_TIMEOUT` |
| **easy handle多线程共享** | 非线程安全 | 每个线程独立 handle |
| **Postfields指向临时对象** | 字符串析构后curl再读取→野指针 | 确保数据在 perform 结束前有效 |
| **curl_slist 未释放** | 内存泄漏 | RAII封装或手动 `curl_slist_free_all` |

**最佳实践：**

- 全局 RAII 封装 `CurlGlobal`，程序启动时初始化一次
- 每个 HTTP 会话用独立的 `CurlEasy` RAII 对象
- 生产环境始终设置超时（连接超时 + 总超时）
- SSL 默认开启；仅在内网测试跳过
- Multi interface 用于需要 >100 并发的高性能场景；简单场景 threads + easy handle 足够

---

## 6. 生产项目典型用法

### 场景：REST API 客户端

```cpp
#include "curl_easy.hpp"   // 以上面的CurlEasy封装

class ApiClient {
public:
    ApiClient(const std::string& base_url, const std::string& token)
        : base_url_(base_url), token_(token) {}

    // GET
    std::string get(const std::string& path) {
        CurlEasy curl;
        curl.set_url(base_url_ + path);
        curl.set_header("Authorization: Bearer " + token_);
        curl.set_header("Accept: application/json");
        curl.set_timeout(30);

        auto [code, response] = curl.perform();
        if (code != CURLE_OK) {
            throw std::runtime_error("GET " + path + " 失败");
        }

        long http_code = curl.get_response_code();
        if (http_code != 200) {
            throw std::runtime_error("HTTP " + std::to_string(http_code));
        }

        return response;
    }

    // POST
    std::string post(const std::string& path, const std::string& json_body) {
        CurlEasy curl;
        curl.set_url(base_url_ + path);
        curl.set_header("Authorization: Bearer " + token_);
        curl.set_header("Content-Type: application/json");
        curl.set_post_fields(json_body);
        curl.set_timeout(30);

        auto [code, response] = curl.perform();
        if (code != CURLE_OK) {
            throw std::runtime_error("POST " + path + " 失败");
        }
        return response;
    }

private:
    std::string base_url_;
    std::string token_;
};

int main() {
    CurlGlobal curl_global;  // 全局初始化

    try {
        ApiClient api("https://api.example.com/v1", "your-access-token");

        std::string users_json = api.get("/users");
        std::cout << "用户列表: " << users_json << "\n";

        std::string result = api.post("/users", R"({"name":"新用户"})");
        std::cout << "创建结果: " << result << "\n";

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

---

## 参考链接

- [libcurl 官方文档](https://curl.se/libcurl/c/)
- [curl_easy_setopt 选项列表](https://curl.se/libcurl/c/curl_easy_setopt.html)
