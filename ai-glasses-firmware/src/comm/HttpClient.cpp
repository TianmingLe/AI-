#include "HttpClient.h"

#include "../core/LogManager.h"

#include <chrono>
#include <dlfcn.h>
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
    last_error_ = err;
    LOG_ERROR("HttpClient: " + err);
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
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_.clear();

    if (use_mock_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        return std::string("{\"status\":\"ok\",\"mock\":true,\"method\":\"get\"}");
    }

    if (!curl_easy_init_ptr_) {
        setError("curl symbols not loaded");
        return std::nullopt;
    }

    void* curl = curl_easy_init_ptr_();
    if (!curl) {
        setError("curl_easy_init returned null");
        return std::nullopt;
    }

    std::string readBuffer;
    if (curl_easy_setopt_ptr_(curl, CURLOPT_URL, url.c_str()) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_WRITEFUNCTION, WriteCallback) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_WRITEDATA, &readBuffer) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_TIMEOUT, 5L) != 0) {
        curl_easy_cleanup_ptr_(curl);
        setError("curl_easy_setopt failed for GET " + url);
        return std::nullopt;
    }

    int res = curl_easy_perform_ptr_(curl);
    curl_easy_cleanup_ptr_(curl);
    if (res != 0) {
        setError("curl_easy_perform failed for GET " + url + " code " + std::to_string(res));
        return std::nullopt;
    }
    return readBuffer;
}

std::optional<std::string> HttpClient::post(const std::string& url, const std::string& json_payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_error_.clear();

    if (use_mock_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        (void)json_payload;
        return std::string("{\"status\":\"ok\",\"mock\":true,\"method\":\"post\"}");
    }

    if (!curl_easy_init_ptr_) {
        setError("curl symbols not loaded");
        return std::nullopt;
    }

    void* curl = curl_easy_init_ptr_();
    if (!curl) {
        setError("curl_easy_init returned null");
        return std::nullopt;
    }

    std::string readBuffer;
    if (curl_easy_setopt_ptr_(curl, CURLOPT_URL, url.c_str()) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_POSTFIELDS, json_payload.c_str()) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_WRITEFUNCTION, WriteCallback) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_WRITEDATA, &readBuffer) != 0 ||
        curl_easy_setopt_ptr_(curl, CURLOPT_TIMEOUT, 5L) != 0) {
        curl_easy_cleanup_ptr_(curl);
        setError("curl_easy_setopt failed for POST " + url);
        return std::nullopt;
    }

    int res = curl_easy_perform_ptr_(curl);
    curl_easy_cleanup_ptr_(curl);
    if (res != 0) {
        setError("curl_easy_perform failed for POST " + url + " code " + std::to_string(res));
        return std::nullopt;
    }
    return readBuffer;
}

} // namespace comm
