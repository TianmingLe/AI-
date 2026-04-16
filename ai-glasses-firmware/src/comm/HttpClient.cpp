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
      last_error_(""),
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

std::string HttpClient::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

void HttpClient::setError(const std::string& err) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_ = err;
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

std::optional<std::string> HttpClient::get(const std::string& url) {
    bool use_mock;
    auto curl_easy_init_ptr = curl_easy_init_ptr_;
    auto curl_easy_cleanup_ptr = curl_easy_cleanup_ptr_;
    auto curl_easy_setopt_ptr = curl_easy_setopt_ptr_;
    auto curl_easy_perform_ptr = curl_easy_perform_ptr_;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_error_.clear();
        use_mock = use_mock_;
    }

    if (use_mock) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        return std::string("{\"status\":\"ok\",\"mock\":true,\"method\":\"get\"}");
    }

    if (!curl_easy_init_ptr || !curl_easy_cleanup_ptr || !curl_easy_setopt_ptr || !curl_easy_perform_ptr) {
        setError("curl symbols not loaded");
        LOG_ERROR("HttpClient: curl symbols not loaded");
        return std::nullopt;
    }

    void* curl = curl_easy_init_ptr();
    if (!curl) {
        setError("curl_easy_init returned null");
        LOG_ERROR("HttpClient: curl_easy_init returned null");
        return std::nullopt;
    }

    std::string readBuffer;
    if (curl_easy_setopt_ptr(curl, CURLOPT_URL, url.c_str()) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEFUNCTION, WriteCallback) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEDATA, &readBuffer) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_TIMEOUT, 5L) != 0) {
        curl_easy_cleanup_ptr(curl);
        setError("curl_easy_setopt failed for GET " + url);
        LOG_ERROR("HttpClient: curl_easy_setopt failed for GET " + url);
        return std::nullopt;
    }

    int res = curl_easy_perform_ptr(curl);
    curl_easy_cleanup_ptr(curl);
    if (res != 0) {
        std::string err = "curl_easy_perform failed for GET " + url + " code " + std::to_string(res);
        setError(err);
        LOG_ERROR("HttpClient: " + err);
        return std::nullopt;
    }
    return readBuffer;
}

std::optional<std::string> HttpClient::post(const std::string& url, const std::string& json_payload) {
    bool use_mock;
    auto curl_easy_init_ptr = curl_easy_init_ptr_;
    auto curl_easy_cleanup_ptr = curl_easy_cleanup_ptr_;
    auto curl_easy_setopt_ptr = curl_easy_setopt_ptr_;
    auto curl_easy_perform_ptr = curl_easy_perform_ptr_;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        last_error_.clear();
        use_mock = use_mock_;
    }

    if (use_mock) {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        (void)json_payload;
        return std::string("{\"status\":\"ok\",\"mock\":true,\"method\":\"post\"}");
    }

    if (!curl_easy_init_ptr || !curl_easy_cleanup_ptr || !curl_easy_setopt_ptr || !curl_easy_perform_ptr) {
        setError("curl symbols not loaded");
        LOG_ERROR("HttpClient: curl symbols not loaded");
        return std::nullopt;
    }

    void* curl = curl_easy_init_ptr();
    if (!curl) {
        setError("curl_easy_init returned null");
        LOG_ERROR("HttpClient: curl_easy_init returned null");
        return std::nullopt;
    }

    std::string readBuffer;
    if (curl_easy_setopt_ptr(curl, CURLOPT_URL, url.c_str()) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_POSTFIELDS, json_payload.c_str()) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEFUNCTION, WriteCallback) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_WRITEDATA, &readBuffer) != 0 ||
        curl_easy_setopt_ptr(curl, CURLOPT_TIMEOUT, 5L) != 0) {
        curl_easy_cleanup_ptr(curl);
        setError("curl_easy_setopt failed for POST " + url);
        LOG_ERROR("HttpClient: curl_easy_setopt failed for POST " + url);
        return std::nullopt;
    }

    int res = curl_easy_perform_ptr(curl);
    curl_easy_cleanup_ptr(curl);
    if (res != 0) {
        std::string err = "curl_easy_perform failed for POST " + url + " code " + std::to_string(res);
        setError(err);
        LOG_ERROR("HttpClient: " + err);
        return std::nullopt;
    }
    return readBuffer;
}

} // namespace comm
