#pragma once

#include <string>

namespace render {

class ArRenderer {
public:
    ArRenderer();
    void renderOverlay(const std::string& label);
};

} // namespace render
