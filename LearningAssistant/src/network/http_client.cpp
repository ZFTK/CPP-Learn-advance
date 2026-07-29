/*
 * http_client.cpp —— HTTP 客户端实现
 *
 * 双后端支持：libcurl（优先） / WinHTTP（Windows回退）
 * 如果安装了 libcurl：target_link_libraries(... CURL::libcurl)
 * 未安装时：使用 Windows WinHTTP API
 */
#include "network/http_client.h"
#include "core/logger.h"

// ---- libcurl 后端 ----
#ifdef HAS_CURL
#include <curl/curl.h>

namespace la {

HttpClient::HttpClient() {
    // curl_global_init: 初始化 libcurl（全局只需调用一次）
    static bool initialized = false;
    if (!initialized) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        initialized = true;
    }
    // curl_easy_init: 创建请求句柄
    m_curl = curl_easy_init();
    LA_LOG_INFO("HttpClient initialized (libcurl backend)");
}

HttpClient::~HttpClient() {
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

size_t HttpClient::headerCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string headerLine(static_cast<char*>(contents), totalSize);
    auto* headers = static_cast<std::map<std::string, std::string>*>(userp);

    auto colon = headerLine.find(':');
    if (colon != std::string::npos) {
        std::string key = headerLine.substr(0, colon);
        std::string value = headerLine.substr(colon + 1);
        // 去除首尾空白
        value.erase(0, value.find_first_not_of(" \r\n\t"));
        value.erase(value.find_last_not_of(" \r\n\t") + 1);
        (*headers)[key] = value;
    }
    return totalSize;
}

void HttpClient::setDefaultHeader(const std::string& key, const std::string& value) {
    m_defaultHeaders[key] = value;
}

void HttpClient::setTimeout(long seconds) {
    m_timeout = seconds;
    if (m_curl) {
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    }
}

HttpResponse HttpClient::get(const std::string& url) {
    HttpResponse response;
    if (!m_curl) {
        response.error = "CURL not initialized";
        return response;
    }

    std::string responseBody;
    std::map<std::string, std::string> responseHeaders;

    // ---- 设置 curl 选项 ----
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &responseHeaders);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);  // 跟随重定向
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 1L);  // 验证 SSL 证书

    // 设置自定义请求头
    struct curl_slist* headers = nullptr;
    for (const auto& [key, value] : m_defaultHeaders) {
        headers = curl_slist_append(headers, (key + ": " + value).c_str());
    }
    if (headers) {
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
    }

    // 执行请求
    CURLcode res = curl_easy_perform(m_curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = std::move(responseBody);
        response.headers = std::move(responseHeaders);
    } else {
        response.error = curl_easy_strerror(res);
        LA_LOG_ERROR("CURL GET failed: " + response.error);
    }

    if (headers) curl_slist_free_all(headers);
    return response;
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                               const std::string& contentType) {
    HttpResponse response;
    if (!m_curl) {
        response.error = "CURL not initialized";
        return response;
    }

    std::string responseBody;
    std::map<std::string, std::string> responseHeaders;

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &responseHeaders);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Content-Type: " + contentType).c_str());
    for (const auto& [key, value] : m_defaultHeaders) {
        headers = curl_slist_append(headers, (key + ": " + value).c_str());
    }
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    CURLcode res = curl_easy_perform(m_curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = std::move(responseBody);
        response.headers = std::move(responseHeaders);
    } else {
        response.error = curl_easy_strerror(res);
    }

    curl_slist_free_all(headers);
    return response;
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body) {
    HttpResponse response;
    if (!m_curl) { response.error = "CURL not initialized"; return response; }

    std::string responseBody;
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);

    CURLcode res = curl_easy_perform(m_curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = responseBody;
    } else {
        response.error = curl_easy_strerror(res);
    }
    return response;
}

HttpResponse HttpClient::del(const std::string& url) {
    HttpResponse response;
    if (!m_curl) { response.error = "CURL not initialized"; return response; }

    std::string responseBody;
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);

    CURLcode res = curl_easy_perform(m_curl);
    if (res == CURLE_OK) {
        long httpCode = 0;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
        response.body = responseBody;
    } else {
        response.error = curl_easy_strerror(res);
    }
    return response;
}

bool HttpClient::downloadFile(const std::string& url, const std::string& filepath) {
    if (!m_curl) return false;

    FILE* file = fopen(filepath.c_str(), "wb");
    if (!file) return false;

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, m_timeout);
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(m_curl);
    fclose(file);

    if (res != CURLE_OK) {
        LA_LOG_ERROR("Download failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }
    return true;
}

} // namespace la

#else
// ==============================================================
// WinHTTP 回退（Windows 平台，无需 libcurl）
// ==============================================================
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#pragma comment(lib, "winhttp.lib")

namespace la {

HttpClient::HttpClient() {
    LA_LOG_INFO("HttpClient initialized (WinHTTP backend)");
}

HttpClient::~HttpClient() {}

size_t HttpClient::writeCallback(void*, size_t, size_t, void*) { return 0; }
size_t HttpClient::headerCallback(void*, size_t, size_t, void*) { return 0; }
void HttpClient::setDefaultHeader(const std::string&, const std::string&) {}
void HttpClient::setTimeout(long seconds) { m_timeout = seconds; }

static std::wstring toWide(const std::string& s) {
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
    return ws;
}

static std::string fromWide(const std::wstring& ws) {
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, nullptr, nullptr);
    return s;
}

static HttpResponse winHttpRequest(const std::string& method, const std::wstring& url,
                                    const std::string& body = "") {
    HttpResponse response;

    // 解析 URL
    URL_COMPONENTS urlComp = {0};
    urlComp.dwStructSize = sizeof(urlComp);
    wchar_t host[256] = {0}, path[1024] = {0};
    urlComp.lpszHostName = host;    urlComp.dwHostNameLength = 255;
    urlComp.lpszUrlPath = path;     urlComp.dwUrlPathLength = 1023;

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        response.error = "Invalid URL";
        return response;
    }

    // 打开会话
    HINTERNET hSession = WinHttpOpen(L"LearningAssistant/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { response.error = "WinHttpOpen failed"; return response; }

    HINTERNET hConnect = WinHttpConnect(hSession, host, urlComp.nPort, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); response.error = "WinHttpConnect failed"; return response; }

    DWORD flags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
        toWide(method).c_str(), path, nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        response.error = "WinHttpOpenRequest failed"; return response;
    }

    // 发送请求
    BOOL result;
    if (!body.empty()) {
        std::string sendBody = body;
        result = WinHttpSendRequest(hRequest,
            L"Content-Type: application/json\r\n", 0,
            (LPVOID)sendBody.c_str(), (DWORD)sendBody.size(),
            (DWORD)sendBody.size(), 0);
    } else {
        result = WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    }

    if (!result) {
        response.error = "WinHttpSendRequest failed";
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        return response;
    }

    // 接收响应
    if (WinHttpReceiveResponse(hRequest, nullptr)) {
        // 获取状态码
        DWORD statusCode = 0, size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
        response.statusCode = static_cast<int>(statusCode);

        // 读取响应体
        DWORD bytesRead = 0;
        char buffer[4096];
        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.body.append(buffer, bytesRead);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return response;
}

HttpResponse HttpClient::get(const std::string& url) {
    return winHttpRequest("GET", toWide(url));
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, const std::string&) {
    return winHttpRequest("POST", toWide(url), body);
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body) {
    return winHttpRequest("PUT", toWide(url), body);
}

HttpResponse HttpClient::del(const std::string& url) {
    return winHttpRequest("DELETE", toWide(url));
}

bool HttpClient::downloadFile(const std::string& url, const std::string& filepath) {
    HttpResponse resp = get(url);
    if (!resp.isOk()) return false;
    FILE* f = fopen(filepath.c_str(), "wb");
    if (!f) return false;
    fwrite(resp.body.data(), 1, resp.body.size(), f);
    fclose(f);
    return true;
}

} // namespace la

#endif // _WIN32
#endif // HAS_CURL
