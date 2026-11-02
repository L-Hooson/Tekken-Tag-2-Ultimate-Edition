#pragma once
#include <cstdint>
#include <limits>

namespace cafe::title::air_timing {
// Source 2D touchdown advances past the held frame; legacy baked arcs do not.
constexpr int32_t landing_frame(int32_t frame, int32_t ground_fall) {
    if (ground_fall <= 0 || ground_fall == std::numeric_limits<int32_t>::max()) return frame;
    return frame <= ground_fall ? ground_fall + 1 : frame;
}
}
