#pragma once

#include <mutex>
#include <optional>
#include <string>

namespace comm {

struct HttpResult {
    std::optional<std::string> body;
    std::string error;
    int code{0};

    bool ok() const { return body.has_value(); }
};

struct HttpClientOptions {
    bool require_https{false};
    bool verify_peer{true};
    bool verify_host{true};
    std::string ca_bundle_path;
};

class HttpClient {
public:
    explicit HttpClient(HttpClientOptions options = {});
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    HttpResult get(const std::string& url);
    HttpResult post(const std::string& url, const std::string& json_payload);

    bool isUsingMock() const;

private:
    bool loadCurlLibrary();
    void unloadCurlLibrary();

    mutable std::mutex mutex_;
    bool use_mock_;
    bool library_loaded_;
    void* lib_handle_;
    HttpClientOptions options_;

    void* (*curl_easy_init_ptr_)();
    void (*curl_easy_cleanup_ptr_)(void*);
    int (*curl_easy_setopt_ptr_)(void*, int, ...);
    int (*curl_easy_perform_ptr_)(void*);
};

} // namespace comm
