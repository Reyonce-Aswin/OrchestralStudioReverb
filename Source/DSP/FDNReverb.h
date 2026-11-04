#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace orchestral::dsp
{
/** Eight-line stereo-feedback delay network.

    - Normalized Hadamard feedback matrix (energy preserving in exact arithmetic).
    - Mutually-prime-ish delay lengths to reduce obvious periodicity.
    - Per-line one-pole damping in the feedback paths.
    - RT60-derived feedback gains, with a practical upper bound for safety.

    This is a first usable algorithmic core, not yet a complete room model.
    Call prepare() before processing and do not resize/reconfigure it on the audio thread.
*/
class FDNReverb
{
public:
    static constexpr std::size_t kLineCount = 8;

    struct Parameters
    {
        float decaySeconds = 2.2f;       // Approximate mid/high-band decay.
        float dampingHz = 9000.0f;       // One-pole low-pass cutoff in feedback paths.
        float inputGain = 0.22f;         // Excitation amount.
        float wetGain = 0.25f;           // Output level; dry/wet is handled by the plugin.
    };

    void prepare(double sampleRate, std::size_t maximumBlockSize);
    void reset() noexcept;
    void setParameters(const Parameters& parameters) noexcept;

    // Processes one stereo sample pair. Inputs and outputs may alias.
    void processSample(float inputLeft, float inputRight,
                       float& outputLeft, float& outputRight) noexcept;

    [[nodiscard]] double getSampleRate() const noexcept { return sampleRate; }
    [[nodiscard]] const std::array<std::size_t, kLineCount>& getDelayLengths() const noexcept
    {
        return delayLengths;
    }

private:
    struct DelayLine
    {
        std::vector<float> buffer;
        std::size_t writeIndex = 0;

        void prepare(std::size_t length);
        void reset() noexcept;
        [[nodiscard]] float read() const noexcept;
        void write(float value) noexcept;
    };

    static void hadamardTransform(std::array<float, kLineCount>& values) noexcept;
    void updateCoefficients() noexcept;

    double sampleRate = 44100.0;
    std::array<DelayLine, kLineCount> lines;
    std::array<std::size_t, kLineCount> delayLengths{};
    std::array<float, kLineCount> dampingState{};
    std::array<float, kLineCount> feedbackGain{};
    std::array<float, kLineCount> lowPassCoefficient{};
    std::array<float, kLineCount> delayed{};
    std::array<float, kLineCount> feedback{};
    Parameters parameters{};
};
} // namespace orchestral::dsp
