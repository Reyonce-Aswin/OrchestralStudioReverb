#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace orchestral::dsp
{
/**
 * Stereo early-reflections generator using a compact tapped-delay network.
 * Stage position subtly changes reflection timing, stereo image and level.
 */
class EarlyReflections
{
public:
    struct Parameters
    {
        float roomSize = 1.0f;
        float preDelayMs = 12.0f;
        int stagePosition = 1; // Must match the Stage Position choice order.
    };

    void prepare(double sampleRate);
    void reset() noexcept;
    void setParameters(const Parameters& newParameters) noexcept;

    void processSample(float inputLeft, float inputRight,
                       float& outputLeft, float& outputRight) noexcept;

private:
    static constexpr std::size_t kTapCount = 10;
    static constexpr double kMaximumBufferSeconds = 0.5;

    float readDelayed(const std::vector<float>& buffer, float delaySamples) const noexcept;

    double sampleRate = 44100.0;
    std::vector<float> leftBuffer;
    std::vector<float> rightBuffer;
    std::size_t writeIndex = 0;

    float roomSize = 1.0f;
    float targetRoomSize = 1.0f;
    float preDelayMs = 12.0f;
    float targetPreDelayMs = 12.0f;
    float stagePan = -0.72f;
    float targetStagePan = -0.72f;
    float stageDepth = 0.72f;
    float targetStageDepth = 0.72f;
    float stageLevel = 1.0f;
    float targetStageLevel = 1.0f;
    float smoothingCoefficient = 0.0f;
};
} // namespace orchestral::dsp
