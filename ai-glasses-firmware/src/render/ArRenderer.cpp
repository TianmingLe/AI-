#include "ArRenderer.h"

#include "../core/LogManager.h"

namespace render {

ArRenderer::ArRenderer() {
    LOG_INFO("ArRenderer: init");
}

void ArRenderer::renderOverlay(const std::string& label) {
    LOG_DEBUG("ArRenderer overlay: " + label);
}

} // namespace render
