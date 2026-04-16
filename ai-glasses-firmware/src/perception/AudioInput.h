#pragma once

#include <string>
#include <vector>

namespace perception {

class AudioInput {
public:
    explicit AudioInput(const std::string& device = "mock");
    bool start();
    void stop();
    std::vector<int16_t> readChunk();
    bool isUsingMock() const;

private:
    bool use_mock_;
    bool running_;
    int tick_;
};

} // namespace perception
