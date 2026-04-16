#include "HttpClient.h"

#include "../core/LogManager.h"

#include <chrono>
#include <dlfcn.h>
#include <utility>
#include <thread>

namespace comm {

constexpr int CURLOPT_URL = 10002;
constexpr int CURLOPT_WRITEFUNCTION = 20011;
constexpr int CURLOPT_WRITEDATA = 10001;
constexpr int CURLOPT_POSTFIELDS = 10015;
constexpr int CURLOPT_TIMEOUT = 13;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

HttpClient::HttpClient()
    : use_mock_(true),
      library_loaded_(false),
      lib_handle_(nullptr),
      curl_easy_init_ptr_(nullptr),
      curl_easy_cleanup_ptr_(nullptr),
      curl_easy_setopt_ptr_(nullptr),
      curl_easy_perform_ptr_(nullptr) {
    if (loadCurlLibrary()) {
        use_mock_ = false;
        LOG_INFO("HttpClient: libcurl loaded");
    } else {
        LOG_WARN("HttpClient: libcurl not available, using mock");
    }
}

HttpClient::~HttpClient() {
    unloadCurlLibrary();
}

bool HttpClient::isUsingMock() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return use_mock_;
}

bool HttpClient::loadCurlLibrary() {
    lib_handle_ = dlopen("libcurl.so.4", RTLD_LAZY);
    if (!lib_handle_) {
        return false;
    }

    curl_easy_init_ptr_ = reinterpret_cast<void* (*)()>(dlsym(lib_handle_, "curl_easy_init"));
    curl_easy_cleanup_ptr_ = reinterpret_cast<void (*)(void*)>(dlsym(lib_handle_, "curl_easy_cleanup"));
    curl_easy_setopt_ptr_ = reinterpret_cast<int (*)(void*, int, ...)>(dlsym(lib_handle_, "curl_easy_setopt"));
    curl_easy_perform_ptr_ = reinterpret_cast<int (*)(void*)>(dlsym(lib_handle_, "curl_easy_perform"));

    if (!curl_easy_init_ptr_ || !curl_easy_cleanup_ptr_ || !curl_easy_setopt_ptr_ || !curl_easy_perform_ptr_) {
        dlclose(lib_handle_);
        lib_handle_ = nullptr;
        curl_easy_init_ptr_ = nullptr;
        curl_easy_cleanup_ptr_ = nullptr;
        curl_easy_setopt_ptr_ = nullptr;
        curl_easy_perform_ptr_ = nullptr;
        return false;
    }

    library_loaded_ = true;
    return true;
}

void HttpClient::unloadCurlLibrary() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (lib_handle_) {
        dlclose(lib_handle_);
        lib_handle_ = nullptr;
    }
    library_loaded_ = false;
    use_mock_ = true;
    curl_easy_init_ptr_ = nullptr;
    curl_easy_cleanup_ptr_ = nullptr;
    curl_easy_setopt_ptr_ = nullptr;
    curl_easy_perform_ptr_ = nullptr;
}

HttpResult HttpClient::get(const std::string& url) {
    bool use_mock;
    auto curl_easy_init_ptr = curl_easy_init_ptr_;
    auto curl_easy_cleanup_ptr = curl_easy_cleanup_ptr_;
    auto curl_easy_setopt_ptr = curl_easy_setopt_ptr_;
    auto curl_easy_perform_ptr = curl_easy_perform_ptr_;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        use_mock = use_mock_;
    }

    if (use_mock) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        HttpResult ok;
        ok.body = std::string("{\"status\":\"ok\",\"mock\":true,\"method\":\"get\"}");
        return ok;
    }

    if (!curl_easy_init_ptr || !curl_easy_cleanup_ptr || !curl_easy_setopt_ptr || !curl_easy_perform_ptr) {
        HttpResult r;
        r.error = "curl symbols not loaded";
        r.code = -1;
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }

    void* curl = curl_easy_init_ptr();
    if (!curl) {
        HttpResult r;
        r.error = "curl_easy_init returned null";
        r.code = -2;
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }

    std::string readBuffer;
    if (curl_easy_setopt_ptr(curl, CURLOPT_URL, url.c_str()) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEFUNCTION, WriteCallback) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEDATA, &readBuffer) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_TIMEOUT, 5L) != 0) {
        curl_easy_cleanup_ptr(curl);
        HttpResult r;
        r.error = "curl_easy_setopt failed for GET " + url;
        r.code = -3;
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }

    int res = curl_easy_perform_ptr(curl);
    curl_easy_cleanup_ptr(curl);
    if (res != 0) {
        HttpResult r;
        r.code = res;
        r.error = "curl_easy_perform failed for GET " + url + " code " + std::to_string(res);
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }
    HttpResult ok;
    ok.body = std::move(readBuffer);
    return ok;
}

HttpResult HttpClient::post(const std::string& url, const std::string& json_payload) {
    bool use_mock;
    auto curl_easy_init_ptr = curl_easy_init_ptr_;
    auto curl_easy_cleanup_ptr = curl_easy_cleanup_ptr_;
    auto curl_easy_setopt_ptr = curl_easy_setopt_ptr_;
    auto curl_easy_perform_ptr = curl_easy_perform_ptr_;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        use_mock = use_mock_;
    }

    if (use_mock) {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        (void)json_payload;
        HttpResult ok;
        ok.body = std::string("{\"status\":\"ok\",\"mock\":true,\"method\":\"post\"}");
        return ok;
    }

    if (!curl_easy_init_ptr || !curl_easy_cleanup_ptr || !curl_easy_setopt_ptr || !curl_easy_perform_ptr) {
        HttpResult r;
        r.error = "curl symbols not loaded";
        r.code = -1;
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }

    void* curl = curl_easy_init_ptr();
    if (!curl) {
        HttpResult r;
        r.error = "curl_easy_init returned null";
        r.code = -2;
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }

    std::string readBuffer;
    if (curl_easy_setopt_ptr(curl, CURLOPT_URL, url.c_str()) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_POSTFIELDS, json_payload.c_str()) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEFUNCTION, WriteCallback) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEDATA, &readBuffer) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_TIMEOUT, 5L) != 0) {
        curl_easy_cleanup_ptr(curl);
        HttpResult r;
        r.error = "curl_easy_setopt failed for POST " + url;
        r.code = -3;
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }

    int res = curl_easy_perform_ptr(curl);
    curl_easy_cleanup_ptr(curl);
    if (res != 0) {
        HttpResult r;
        r.code = res;
        r.error = "curl_easy_perform failed for POST " + url + " code " + std::to_string(res);
        LOG_ERROR("HttpClient: " + r.error);
        return r;
    }
    HttpResult ok;
    ok.body = std::move(readBuffer);
    return ok;
}

} // namespace comm
