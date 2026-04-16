#pragma once

#include <mutex>
#include <optional>
#include <string>

namespace comm {

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    std::optional<std::string> get(const std::string& url);
    std::optional<std::string> post(const std::string& url, const std::string& json_payload);

    bool isUsingMock() const;
    std::string getLastError() const;

private:
    bool loadCurlLibrary();
    void unloadCurlLibrary();
    void setError(const std::string& err);

    mutable std::mutex mutex_;
    bool use_mock_;
    bool library_loaded_;
    std::string last_error_;
    void* lib_handle_;

    void* (*curl_easy_init_ptr_)();
    void (*curl_easy_cleanup_ptr_)(void*);
    int (*curl_easy_setopt_ptr_)(void*, int, ...);
    int (*curl_easy_perform_ptr_)(void*);
};

} // namespace comm
