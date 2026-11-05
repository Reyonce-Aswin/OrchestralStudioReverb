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

// Position order mirrors the JUCE Stage Position choice list:
// Piano, strings, woodwinds, percussion, horns, then brass.
struct StageProfile { float pan; float depth; float level; };
constexpr std::array<StageProfile, 19> kStageProfiles{{
    {  0.00f, 0.72f, 1.00f }, // Piano
    { -0.78f, 0.62f, 1.00f }, // Violin I
    { -0.48f, 0.66f, 1.00f }, // Violin II
    { -0.16f, 0.70f, 1.00f }, // Violas
    {  0.30f, 0.77f, 0.96f }, // Celli
    {  0.70f, 0.84f, 0.92f }, // Basses
    { -0.78f, 0.53f, 0.98f }, // Piccolos
    { -0.48f, 0.56f, 0.98f }, // Flutes
    { -0.18f, 0.60f, 0.98f }, // Oboes
    {  0.16f, 0.64f, 0.98f }, // Clarinets
    {  0.52f, 0.69f, 0.96f }, // Bassoons
    { -0.82f, 1.00f, 0.86f }, // Horns
    { -0.66f, 0.90f, 0.90f }, // Harp / Celeste
    {  0.00f, 1.00f, 0.88f }, // Timpani / Untuned Percussion
    {  0.68f, 0.96f, 0.88f }, // Tubular bells / mallets
    {  0.28f, 1.02f, 0.84f }, // Trumpets
    {  0.42f, 1.06f, 0.82f }, // Tenor Trombones
    {  0.56f, 1.10f, 0.80f }, // Bass Trombones
    {  0.70f, 1.14f, 0.78f }  // Tuba
}};
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
    smoothingCoefficient = 1.0f - std::exp(
        -1.0f / static_cast<float>(0.050 * sampleRate));
    roomSize = targetRoomSize;
    preDelayMs = targetPreDelayMs;
    stagePan = targetStagePan;
    stageDepth = targetStageDepth;
    stageLevel = targetStageLevel;
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
    const auto index = static_cast<std::size_t>(
        std::clamp(newParameters.stagePosition, 0, static_cast<int>(kStageProfiles.size()) - 1));
    targetStagePan = kStageProfiles[index].pan;
    targetStageDepth = kStageProfiles[index].depth;
    targetStageLevel = kStageProfiles[index].level;
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

    leftBuffer[writeIndex] = inputLeft;
    rightBuffer[writeIndex] = inputRight;

    roomSize += smoothingCoefficient * (targetRoomSize - roomSize);
    preDelayMs += smoothingCoefficient * (targetPreDelayMs - preDelayMs);
    stagePan += smoothingCoefficient * (targetStagePan - stagePan);
    stageDepth += smoothingCoefficient * (targetStageDepth - stageDepth);
    stageLevel += smoothingCoefficient * (targetStageLevel - stageLevel);

    float left = 0.0f;
    float right = 0.0f;
    const float depthScale = 0.82f + 0.30f * stageDepth;

    for (std::size_t tap = 0; tap < kTapCount; ++tap)
    {
        const float delayMs = preDelayMs + kTapTimesMs[tap] * roomSize * depthScale;
        const float delaySamples = delayMs * 0.001f * static_cast<float>(sampleRate);
        const float delayedLeft = readDelayed(leftBuffer, delaySamples);
        const float delayedRight = readDelayed(rightBuffer, delaySamples);

        const float pan = std::clamp(kTapPan[tap] * 0.58f + stagePan * 0.42f, -0.96f, 0.96f);
        const float leftPan = std::sqrt(0.5f * (1.0f - pan));
        const float rightPan = std::sqrt(0.5f * (1.0f + pan));
        const float gain = kTapGains[tap] * stageLevel;

        left += gain * (delayedLeft * leftPan
                      + delayedRight * rightPan * kCrossfeed);
        right += gain * (delayedRight * rightPan
                       + delayedLeft * leftPan * kCrossfeed);
    }

    // A subtle equal-power stage bias makes left/right positions distinguishable
    // without hard-panning the entire wet return.
    const float leftBias = std::sqrt(0.5f * (1.0f - stagePan * 0.22f)) * 1.41421356f;
    const float rightBias = std::sqrt(0.5f * (1.0f + stagePan * 0.22f)) * 1.41421356f;
    outputLeft = left * kOutputScale * leftBias;
    outputRight = right * kOutputScale * rightBias;

    if (++writeIndex == leftBuffer.size())
        writeIndex = 0;
}
} // namespace orchestral::dsp
