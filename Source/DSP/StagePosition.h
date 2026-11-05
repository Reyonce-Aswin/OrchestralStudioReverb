#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace orchestral::dsp
{
enum class StageLayout : int
{
    fullModern = 0,
    fullClassical = 1,
    chamberModern = 2,
    chamberClassical = 3
};

struct StagePosition
{
    float pan = 0.0f;       // -1 = far left, +1 = far right
    float depth = 0.5f;     // 0 = front/baseline, 1 = rear of stage
    float level = 1.0f;     // gentle distance compensation
};

struct PolarStagePosition
{
    float radius;
    float angleDegrees;
    float maximumRadius;
    float level;
};

inline StagePosition makeStagePosition(const PolarStagePosition& p) noexcept
{
    constexpr float pi = 3.14159265358979323846f;
    const float angle = p.angleDegrees * pi / 180.0f;
    return {
        std::clamp(std::cos(angle), -1.0f, 1.0f),
        std::clamp(p.radius / p.maximumRadius, 0.0f, 1.0f),
        p.level
    };
}

// Index order is deliberately identical to the persistent stagePosition
// parameter choice list. Coordinates are sector-centre polar positions
// derived from each approved seating layout's ring radii and angle ranges.
inline StagePosition getStagePosition(StageLayout layout, int instrumentIndex) noexcept
{
    using P = PolarStagePosition;
    static constexpr std::array<P, 19> fullModern{{
        { 81, 90, 388, 1.00f }, { 180.5f, 161, 388, 1.00f },
        { 151, 116, 388, 1.00f }, { 151, 65, 388, 1.00f },
        { 162.5f, 20, 388, 0.98f }, { 270.5f, 20, 388, 0.94f },
        { 228, 131.5f, 388, 0.98f }, { 228, 110.5f, 388, 0.98f },
        { 228, 89.5f, 388, 0.98f }, { 228, 68.5f, 388, 0.98f },
        { 228, 49, 388, 0.96f }, { 290, 161.5f, 388, 0.86f },
        { 290, 127, 388, 0.90f }, { 290, 90, 388, 0.88f },
        { 290, 54.5f, 388, 0.88f }, { 357, 35, 388, 0.84f },
        { 357, 25, 388, 0.82f }, { 357, 16, 388, 0.80f },
        { 357, 6, 388, 0.78f }
    }};
    static constexpr std::array<P, 19> fullClassical{{
        { 81, 90, 385, 1.00f }, { 156.5f, 160, 385, 1.00f },
        { 156.5f, 20, 385, 1.00f }, { 156.5f, 65, 385, 1.00f },
        { 156.5f, 115, 385, 0.98f }, { 239, 134, 385, 0.94f },
        { 239, 106, 385, 0.98f }, { 239, 106, 385, 0.98f },
        { 239, 82, 385, 0.98f }, { 239, 58, 385, 0.98f },
        { 239, 34, 385, 0.96f }, { 299, 134, 385, 0.86f },
        { 299, 166, 385, 0.90f }, { 299, 90, 385, 0.88f },
        { 299, 53, 385, 0.88f }, { 299, 109, 385, 0.84f },
        { 299, 88, 385, 0.82f }, { 299, 69, 385, 0.80f },
        { 299, 52, 385, 0.78f }
    }};
    static constexpr std::array<P, 19> chamberModern{{
        { 81, 90, 345, 1.00f }, { 136.5f, 157.5f, 345, 1.00f },
        { 136.5f, 112.5f, 345, 1.00f }, { 136.5f, 67.5f, 345, 1.00f },
        { 136.5f, 22.5f, 345, 0.98f }, { 199, 22.5f, 345, 0.94f },
        { 199, 163, 345, 0.98f }, { 199, 163, 345, 0.98f },
        { 199, 129, 345, 0.98f }, { 199, 95, 345, 0.98f },
        { 199, 61.5f, 345, 0.96f }, { 259, 129, 345, 0.88f },
        { 81, 90, 345, 0.90f }, { 319, 90, 345, 0.88f },
        { 319, 90, 345, 0.88f }, { 259, 87, 345, 0.84f },
        { 259, 87, 345, 0.82f }, { 259, 87, 345, 0.80f },
        { 319, 90, 345, 0.78f }
    }};
    static constexpr std::array<P, 19> chamberClassical{{
        { 81, 90, 345, 1.00f }, { 136.5f, 160, 345, 1.00f },
        { 136.5f, 20, 345, 1.00f }, { 136.5f, 65, 345, 1.00f },
        { 136.5f, 115, 345, 0.98f }, { 199, 136, 345, 0.94f },
        { 199, 110, 345, 0.98f }, { 199, 110, 345, 0.98f },
        { 199, 86, 345, 0.98f }, { 199, 62, 345, 0.98f },
        { 199, 38, 345, 0.96f }, { 259, 111, 345, 0.88f },
        { 81, 90, 345, 0.90f }, { 319, 90, 345, 0.88f },
        { 319, 90, 345, 0.88f }, { 259, 81, 345, 0.84f },
        { 259, 81, 345, 0.82f }, { 259, 81, 345, 0.80f },
        { 319, 90, 345, 0.78f }
    }};

    const int index = std::clamp(instrumentIndex, 0, 18);
    switch (layout)
    {
        case StageLayout::fullClassical: return makeStagePosition(fullClassical[static_cast<std::size_t>(index)]);
        case StageLayout::chamberModern: return makeStagePosition(chamberModern[static_cast<std::size_t>(index)]);
        case StageLayout::chamberClassical: return makeStagePosition(chamberClassical[static_cast<std::size_t>(index)]);
        case StageLayout::fullModern:
        default: return makeStagePosition(fullModern[static_cast<std::size_t>(index)]);
    }
}
} // namespace orchestral::dsp
