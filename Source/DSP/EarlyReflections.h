#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace orchestral::dsp
{
/**
 * Stereo early-reflections generator using a compact tapped-delay network.
 *
 * The delay buffers are allocated in prepare(). processSample() performs no
 * allocation or locking and is safe to call from the audio thread.
 */
class EarlyReflections
{
public:
    struct Parameters
    {
        float roomSize = 1.0f;       // Delay-time scale: 0.5 (small) to 2.0 (large).
        float preDelayMs = 12.0f;    // Silence before the first early reflection.
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
    float smoothingCoefficient = 0.0f;
};
} // namespace orchestral::dsp
