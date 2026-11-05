#include "FDNReverb.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace orchestral::dsp
{
namespace
{
constexpr std::array<double, FDNReverb::kLineCount> kDelaySeconds{
    0.0297, 0.0371, 0.0411, 0.0437, 0.0531, 0.0593, 0.0617, 0.0713
};
constexpr float kInvSqrt8 = 0.3535533905932738f;
constexpr float kInvSqrt4 = 0.5f;
constexpr float kPi = 3.14159265358979323846f;

struct StageProfile { float pan; float depth; };
constexpr std::array<StageProfile, 19> kStageProfiles{{
    {  0.00f, 0.72f }, // Piano
    { -0.78f, 0.62f }, // Violin I
    { -0.48f, 0.66f }, // Violin II
    { -0.16f, 0.70f }, // Violas
    {  0.30f, 0.77f }, // Celli
    {  0.70f, 0.84f }, // Basses
    { -0.78f, 0.53f }, // Piccolos
    { -0.48f, 0.56f }, // Flutes
    { -0.18f, 0.60f }, // Oboes
    {  0.16f, 0.64f }, // Clarinets
    {  0.52f, 0.69f }, // Bassoons
    { -0.82f, 1.00f }, // Horns
    { -0.66f, 0.90f }, // Harp / Celeste
    {  0.00f, 1.00f }, // Timpani / Untuned Percussion
    {  0.68f, 0.96f }, // Tubular bells / mallets
    {  0.28f, 1.02f }, // Trumpets
    {  0.42f, 1.06f }, // Tenor Trombones
    {  0.56f, 1.10f }, // Bass Trombones
    {  0.70f, 1.14f }  // Tuba
}};
}

void FDNReverb::DelayLine::prepare(std::size_t length)
{
    buffer.assign(std::max<std::size_t>(length, 1), 0.0f);
    writeIndex = 0;
}

void FDNReverb::DelayLine::reset() noexcept
{
    std::fill(buffer.begin(), buffer.end(), 0.0f);
    writeIndex = 0;
}

float FDNReverb::DelayLine::read() const noexcept
{
    return buffer[writeIndex];
}

void FDNReverb::DelayLine::write(float value) noexcept
{
    buffer[writeIndex] = value;
    if (++writeIndex == buffer.size())
        writeIndex = 0;
}

void FDNReverb::prepare(double newSampleRate, std::size_t /*maximumBlockSize*/)
{
    if (!(newSampleRate > 0.0))
        throw std::invalid_argument("FDNReverb requires a positive sample rate");

    sampleRate = newSampleRate;
    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        delayLengths[i] = std::max<std::size_t>(2,
            static_cast<std::size_t>(std::round(kDelaySeconds[i] * sampleRate)));
        lines[i].prepare(delayLengths[i]);
    }
    reset();
    updateCoefficients();
}

void FDNReverb::reset() noexcept
{
    for (auto& line : lines)
        line.reset();
    dampingState.fill(0.0f);
    delayed.fill(0.0f);
    feedback.fill(0.0f);
}

void FDNReverb::setParameters(const Parameters& newParameters) noexcept
{
    parameters.decaySeconds = std::clamp(newParameters.decaySeconds, 0.15f, 20.0f);
    parameters.dampingHz = std::clamp(newParameters.dampingHz, 500.0f,
                                      static_cast<float>(sampleRate * 0.45));
    parameters.inputGain = std::clamp(newParameters.inputGain, 0.0f, 1.0f);
    parameters.wetGain = std::clamp(newParameters.wetGain, 0.0f, 2.0f);
    parameters.stagePosition = std::clamp(newParameters.stagePosition, 0,
                                          static_cast<int>(kStageProfiles.size()) - 1);
    stagePan = kStageProfiles[static_cast<std::size_t>(parameters.stagePosition)].pan;
    stageDepth = kStageProfiles[static_cast<std::size_t>(parameters.stagePosition)].depth;
    updateCoefficients();
}

void FDNReverb::updateCoefficients() noexcept
{
    const float cutoff = std::clamp(parameters.dampingHz, 20.0f,
                                    static_cast<float>(sampleRate * 0.45));
    const float alpha = 1.0f - std::exp(-2.0f * kPi * cutoff / static_cast<float>(sampleRate));

    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        lowPassCoefficient[i] = alpha;
        const float delaySeconds = static_cast<float>(delayLengths[i] / sampleRate);
        // -60 dB amplitude after decaySeconds: 10^(-3 * delay / RT60).
        feedbackGain[i] = std::min(0.9995f,
            std::pow(10.0f, -3.0f * delaySeconds / parameters.decaySeconds));
    }
}

void FDNReverb::hadamardTransform(std::array<float, kLineCount>& v) noexcept
{
    for (std::size_t stride = 1; stride < kLineCount; stride *= 2)
    {
        for (std::size_t base = 0; base < kLineCount; base += 2 * stride)
        {
            for (std::size_t j = 0; j < stride; ++j)
            {
                const float a = v[base + j];
                const float b = v[base + j + stride];
                v[base + j] = a + b;
                v[base + j + stride] = a - b;
            }
        }
    }
    for (auto& value : v)
        value *= kInvSqrt8;
}

void FDNReverb::processSample(float inputLeft, float inputRight,
                              float& outputLeft, float& outputRight) noexcept
{
    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        delayed[i] = lines[i].read();
        dampingState[i] += lowPassCoefficient[i] * (delayed[i] - dampingState[i]);
        feedback[i] = dampingState[i];
    }

    hadamardTransform(feedback);

    constexpr std::array<float, kLineCount> leftPattern{ 1, 1, 1, 1, 0, 0, 0, 0 };
    constexpr std::array<float, kLineCount> rightPattern{ 0, 0, 0, 0, 1, 1, 1, 1 };

    // Stage position changes how strongly each side excites the network.
    // Depth also changes the balance between the near and far line groups.
    const float panLeft = std::sqrt(0.5f * (1.0f - stagePan));
    const float panRight = std::sqrt(0.5f * (1.0f + stagePan));
    const float nearWeight = 1.08f - 0.16f * stageDepth;
    const float farWeight = 0.84f + 0.16f * stageDepth;

    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        const bool nearGroup = i < 4;
        const float depthWeight = nearGroup ? nearWeight : farWeight;
        const float excitation = kInvSqrt4 * depthWeight
            * (leftPattern[i] * inputLeft * panLeft * 1.41421356f
             + rightPattern[i] * inputRight * panRight * 1.41421356f);
        lines[i].write(feedbackGain[i] * feedback[i] + parameters.inputGain * excitation);
    }

    constexpr std::array<float, kLineCount> leftOutput{ 1, 1, -1, -1,  1, 1, -1, -1 };
    constexpr std::array<float, kLineCount> rightOutput{ 1, -1, 1, -1,  1, -1, 1, -1 };
    float l = 0.0f;
    float r = 0.0f;
    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        l += leftOutput[i] * delayed[i];
        r += rightOutput[i] * delayed[i];
    }

    const float width = 0.88f + 0.12f * std::clamp(stageDepth, 0.0f, 1.2f);
    const float leftPerspective = std::sqrt(1.0f - 0.12f * stagePan);
    const float rightPerspective = std::sqrt(1.0f + 0.12f * stagePan);
    outputLeft = parameters.wetGain * l * kInvSqrt8 * width * leftPerspective;
    outputRight = parameters.wetGain * r * kInvSqrt8 * width * rightPerspective;
}
} // namespace orchestral::dsp
