#include "AudioInput.h"

#include "../core/LogManager.h"

namespace perception {

AudioInput::AudioInput(const std::string&) : use_mock_(true), running_(false), tick_(0) {
    LOG_INFO("AudioInput: using mock");
}

bool AudioInput::start() {
    running_ = true;
    return true;
}

void AudioInput::stop() {
    running_ = false;
}

std::vector<int16_t> AudioInput::readChunk() {
    if (!running_) return {};
    tick_++;
    std::vector<int16_t> chunk(160);
    for (size_t i = 0; i < chunk.size(); i++) {
        chunk[i] = static_cast<int16_t>((tick_ + static_cast<int>(i)) % 100);
    }
    return chunk;
}

bool AudioInput::isUsingMock() const {
    return use_mock_;
}

} // namespace perception
