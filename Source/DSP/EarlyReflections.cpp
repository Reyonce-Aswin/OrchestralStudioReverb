#include "EarlyReflections.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace orchestral::dsp
{
namespace
{
constexpr std::array<float, 10> kTapTimesMs{
    8.1f, 11.7f, 17.3f, 23.9f, 31.1f,
    39.7f, 48.9f, 59.3f, 71.7f, 86.3f
};

constexpr std::array<float, 10> kTapGains{
    0.62f, 0.56f, 0.50f, 0.44f, 0.38f,
    0.33f, 0.28f, 0.24f, 0.20f, 0.16f
};

constexpr std::array<float, 10> kTapPan{
    -0.82f, 0.68f, -0.48f, 0.42f, -0.18f,
     0.88f, -0.66f, 0.24f, 0.61f, -0.34f
};

constexpr float kCrossfeed = 0.16f;
constexpr float kOutputScale = 0.34f;
}

void EarlyReflections::prepare(double newSampleRate)
{
    if (!(newSampleRate > 0.0))
        throw std::invalid_argument("EarlyReflections requires a positive sample rate");

    sampleRate = newSampleRate;
    const auto capacity = static_cast<std::size_t>(
        std::ceil(sampleRate * kMaximumBufferSeconds)) + 4;
    leftBuffer.assign(capacity, 0.0f);
    rightBuffer.assign(capacity, 0.0f);
    writeIndex = 0;

    // Smooth room-size and pre-delay changes over approximately 25 ms.
    smoothingCoefficient = 1.0f - std::exp(
        -1.0f / static_cast<float>(0.025 * sampleRate));
    roomSize = targetRoomSize;
    preDelayMs = targetPreDelayMs;
}

void EarlyReflections::reset() noexcept
{
    std::fill(leftBuffer.begin(), leftBuffer.end(), 0.0f);
    std::fill(rightBuffer.begin(), rightBuffer.end(), 0.0f);
    writeIndex = 0;
}

void EarlyReflections::setParameters(const Parameters& newParameters) noexcept
{
    targetRoomSize = std::clamp(newParameters.roomSize, 0.5f, 2.0f);
    targetPreDelayMs = std::clamp(newParameters.preDelayMs, 0.0f, 100.0f);
}

float EarlyReflections::readDelayed(const std::vector<float>& buffer,
                                    float delaySamples) const noexcept
{
    const auto size = static_cast<float>(buffer.size());
    const float safeDelay = std::clamp(delaySamples, 1.0f, size - 2.0f);
    float readPosition = static_cast<float>(writeIndex) - safeDelay;
    while (readPosition < 0.0f)
        readPosition += size;

    const auto index0 = static_cast<std::size_t>(readPosition);
    const auto index1 = (index0 + 1) % buffer.size();
    const float fraction = readPosition - static_cast<float>(index0);
    return buffer[index0] + fraction * (buffer[index1] - buffer[index0]);
}

void EarlyReflections::processSample(float inputLeft, float inputRight,
                                     float& outputLeft, float& outputRight) noexcept
{
    if (leftBuffer.empty())
    {
        outputLeft = 0.0f;
        outputRight = 0.0f;
        return;
    }

    // Store the current input before reading taps; all tap delays are >= 1 sample.
    leftBuffer[writeIndex] = inputLeft;
    rightBuffer[writeIndex] = inputRight;

    roomSize += smoothingCoefficient * (targetRoomSize - roomSize);
    preDelayMs += smoothingCoefficient * (targetPreDelayMs - preDelayMs);

    float left = 0.0f;
    float right = 0.0f;
    const float preDelaySamples = preDelayMs * 0.001f * static_cast<float>(sampleRate);

    for (std::size_t tap = 0; tap < kTapCount; ++tap)
    {
        const float delayMs = preDelayMs + kTapTimesMs[tap] * roomSize;
        const float delaySamples = delayMs * 0.001f * static_cast<float>(sampleRate);
        const float delayedLeft = readDelayed(leftBuffer, delaySamples);
        const float delayedRight = readDelayed(rightBuffer, delaySamples);

        const float pan = kTapPan[tap];
        const float leftPan = std::sqrt(0.5f * (1.0f - pan));
        const float rightPan = std::sqrt(0.5f * (1.0f + pan));
        const float gain = kTapGains[tap];

        left += gain * (delayedLeft * leftPan
                      + delayedRight * rightPan * kCrossfeed);
        right += gain * (delayedRight * rightPan
                       + delayedLeft * leftPan * kCrossfeed);
    }

    outputLeft = left * kOutputScale;
    outputRight = right * kOutputScale;

    if (++writeIndex == leftBuffer.size())
        writeIndex = 0;

    (void) preDelaySamples; // Kept separate above for readability of the control model.
}
} // namespace orchestral::dsp
