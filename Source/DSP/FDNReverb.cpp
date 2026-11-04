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
        // A tiny safety margin keeps the feedback loop comfortably bounded.
        feedbackGain[i] = std::min(0.9995f,
            std::pow(10.0f, -3.0f * delaySeconds / parameters.decaySeconds));
    }
}

void FDNReverb::hadamardTransform(std::array<float, kLineCount>& v) noexcept
{
    // In-place 8-point Walsh-Hadamard transform. Normalization makes the
    // matrix orthogonal: H * H^T = I (within floating-point precision).
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
    // Read all delay outputs before writing any line.
    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        delayed[i] = lines[i].read();
        dampingState[i] += lowPassCoefficient[i] * (delayed[i] - dampingState[i]);
        feedback[i] = dampingState[i];
    }

    hadamardTransform(feedback);

    // Two orthogonal-ish excitation patterns preserve stereo distinction while
    // allowing the feedback matrix to create a shared, diffuse field.
    constexpr std::array<float, kLineCount> leftPattern{
        1, 1, 1, 1, 0, 0, 0, 0
    };
    constexpr std::array<float, kLineCount> rightPattern{
        0, 0, 0, 0, 1, 1, 1, 1
    };

    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        const float excitation = kInvSqrt4 *
            (leftPattern[i] * inputLeft + rightPattern[i] * inputRight);
        lines[i].write(feedbackGain[i] * feedback[i] + parameters.inputGain * excitation);
    }

    // Different signed projections produce two related but non-identical
    // stereo perspectives from the same diffuse field.
    constexpr std::array<float, kLineCount> leftOutput{
         1, 1, -1, -1,  1, 1, -1, -1
    };
    constexpr std::array<float, kLineCount> rightOutput{
         1, -1, 1, -1,  1, -1, 1, -1
    };
    float l = 0.0f;
    float r = 0.0f;
    for (std::size_t i = 0; i < kLineCount; ++i)
    {
        l += leftOutput[i] * delayed[i];
        r += rightOutput[i] * delayed[i];
    }
    outputLeft = parameters.wetGain * l * kInvSqrt8;
    outputRight = parameters.wetGain * r * kInvSqrt8;
}
} // namespace orchestral::dsp
