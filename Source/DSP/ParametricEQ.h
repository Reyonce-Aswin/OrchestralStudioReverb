#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace orchestral::dsp
{
class ParametricEQ
{
public:
    enum class Type { LowCut = 0, LowShelf = 1, Bell = 2, HighShelf = 3, HighCut = 4 };

    struct Band
    {
        Type type = Type::Bell;
        float frequency = 1000.0f;
        float gainDb = 0.0f;
        float q = 0.7071f;
        int slope = 1; // 0..3 = 12, 24, 36, 48 dB/oct for cut filters
        bool enabled = false;
    };

    void prepare(double newSampleRate)
    {
        sampleRate = std::max(1000.0, newSampleRate);
        const Coeff identity {};
        for (auto& band : filters)
            for (auto& stage : band)
                for (auto& channel : stage)
                {
                    channel.current = identity;
                    channel.target = identity;
                    channel.remaining = 0;
                    channel.z1 = channel.z2 = 0.0f;
                }
        prepared = true;
    }

    void reset()
    {
        for (auto& band : filters)
            for (auto& stage : band)
                for (auto& channel : stage)
                    channel.z1 = channel.z2 = 0.0f;
    }

    void setBand(int index, const Band& parameters)
    {
        if (!prepared || index < 0 || index >= static_cast<int>(filters.size()))
            return;

        const auto stageCount = parameters.type == Type::LowCut || parameters.type == Type::HighCut
            ? std::clamp(parameters.slope + 1, 1, 4) : 1;
        const auto stageQs = butterworthQs[static_cast<std::size_t>(stageCount - 1)];
        const float resonance = std::clamp(parameters.q / 0.7071f, 0.5f, 2.5f);
        const float frequency = std::clamp(parameters.frequency, 20.0f,
                                            static_cast<float>(sampleRate * 0.45));
        const int smoothingSamples = std::max(1, static_cast<int>(sampleRate * 0.012));

        for (int stage = 0; stage < 4; ++stage)
        {
            Coeff target {};
            if (parameters.enabled && stage < stageCount)
            {
                const float q = (parameters.type == Type::LowCut || parameters.type == Type::HighCut)
                    ? std::clamp(stageQs[static_cast<std::size_t>(stage)] * resonance, 0.25f, 3.0f)
                    : std::clamp(parameters.q, 0.1f, 12.0f);
                target = makeCoefficients(parameters.type, frequency, parameters.gainDb, q);
            }

            for (auto& channel : filters[static_cast<std::size_t>(index)][static_cast<std::size_t>(stage)])
            {
                const bool unchanged = std::abs(channel.target.b0 - target.b0) < 1.0e-7f
                    && std::abs(channel.target.b1 - target.b1) < 1.0e-7f
                    && std::abs(channel.target.b2 - target.b2) < 1.0e-7f
                    && std::abs(channel.target.a1 - target.a1) < 1.0e-7f
                    && std::abs(channel.target.a2 - target.a2) < 1.0e-7f;
                if (unchanged)
                    continue;
                channel.target = target;
                channel.remaining = smoothingSamples;
                channel.step.b0 = (target.b0 - channel.current.b0) / static_cast<float>(smoothingSamples);
                channel.step.b1 = (target.b1 - channel.current.b1) / static_cast<float>(smoothingSamples);
                channel.step.b2 = (target.b2 - channel.current.b2) / static_cast<float>(smoothingSamples);
                channel.step.a1 = (target.a1 - channel.current.a1) / static_cast<float>(smoothingSamples);
                channel.step.a2 = (target.a2 - channel.current.a2) / static_cast<float>(smoothingSamples);
            }
        }
    }

    void processSample(float& left, float& right) noexcept
    {
        for (auto& band : filters)
            for (auto& stage : band)
            {
                left = processOne(left, stage[0]);
                right = processOne(right, stage[1]);
            }
    }

private:
    struct Coeff
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    };
    struct Channel
    {
        Coeff current, target, step;
        float z1 = 0.0f, z2 = 0.0f;
        int remaining = 0;
    };
    using StereoStage = std::array<Channel, 2>;
    using BandStages = std::array<StereoStage, 4>;

    static constexpr std::array<std::array<float, 4>, 4> butterworthQs {{
        {{ 0.70710678f, 0.0f, 0.0f, 0.0f }},
        {{ 0.54119610f, 1.3065630f, 0.0f, 0.0f }},
        {{ 0.51763809f, 0.70710678f, 1.9318517f, 0.0f }},
        {{ 0.50979558f, 0.60134489f, 0.89997622f, 2.5629154f }}
    }};

    Coeff makeCoefficients(Type type, float frequency, float gainDb, float q) const noexcept
    {
        constexpr float pi = 3.14159265358979323846f;
        const float w0 = 2.0f * pi * frequency / static_cast<float>(sampleRate);
        const float cosine = std::cos(w0);
        const float sine = std::sin(w0);
        const float alpha = sine / (2.0f * std::max(0.1f, q));
        const float amplitude = std::pow(10.0f, std::clamp(gainDb, -24.0f, 24.0f) / 40.0f);
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

        switch (type)
        {
            case Type::LowCut:
                b0 = (1.0f + cosine) * 0.5f;
                b1 = -(1.0f + cosine);
                b2 = (1.0f + cosine) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosine;
                a2 = 1.0f - alpha;
                break;
            case Type::HighCut:
                b0 = (1.0f - cosine) * 0.5f;
                b1 = 1.0f - cosine;
                b2 = (1.0f - cosine) * 0.5f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosine;
                a2 = 1.0f - alpha;
                break;
            case Type::Bell:
            {
                b0 = 1.0f + alpha * amplitude;
                b1 = -2.0f * cosine;
                b2 = 1.0f - alpha * amplitude;
                a0 = 1.0f + alpha / amplitude;
                a1 = -2.0f * cosine;
                a2 = 1.0f - alpha / amplitude;
                break;
            }
            case Type::LowShelf:
            case Type::HighShelf:
            {
                const float shelfAlpha = (sine * 0.5f) * std::sqrt(2.0f);
                const float root = 2.0f * std::sqrt(amplitude) * shelfAlpha;
                if (type == Type::LowShelf)
                {
                    b0 = amplitude * ((amplitude + 1.0f) - (amplitude - 1.0f) * cosine + root);
                    b1 = 2.0f * amplitude * ((amplitude - 1.0f) - (amplitude + 1.0f) * cosine);
                    b2 = amplitude * ((amplitude + 1.0f) - (amplitude - 1.0f) * cosine - root);
                    a0 = (amplitude + 1.0f) + (amplitude - 1.0f) * cosine + root;
                    a1 = -2.0f * ((amplitude - 1.0f) + (amplitude + 1.0f) * cosine);
                    a2 = (amplitude + 1.0f) + (amplitude - 1.0f) * cosine - root;
                }
                else
                {
                    b0 = amplitude * ((amplitude + 1.0f) + (amplitude - 1.0f) * cosine + root);
                    b1 = -2.0f * amplitude * ((amplitude - 1.0f) + (amplitude + 1.0f) * cosine);
                    b2 = amplitude * ((amplitude + 1.0f) + (amplitude - 1.0f) * cosine - root);
                    a0 = (amplitude + 1.0f) - (amplitude - 1.0f) * cosine + root;
                    a1 = 2.0f * ((amplitude - 1.0f) - (amplitude + 1.0f) * cosine);
                    a2 = (amplitude + 1.0f) - (amplitude - 1.0f) * cosine - root;
                }
                break;
            }
        }

        const float inverseA0 = 1.0f / a0;
        return { b0 * inverseA0, b1 * inverseA0, b2 * inverseA0,
                 a1 * inverseA0, a2 * inverseA0 };
    }

    static float processOne(float input, Channel& channel) noexcept
    {
        if (channel.remaining > 0)
        {
            channel.current.b0 += channel.step.b0;
            channel.current.b1 += channel.step.b1;
            channel.current.b2 += channel.step.b2;
            channel.current.a1 += channel.step.a1;
            channel.current.a2 += channel.step.a2;
            --channel.remaining;
            if (channel.remaining == 0)
                channel.current = channel.target;
        }
        const auto& c = channel.current;
        const float output = c.b0 * input + channel.z1;
        channel.z1 = c.b1 * input - c.a1 * output + channel.z2;
        channel.z2 = c.b2 * input - c.a2 * output;
        return std::isfinite(output) ? output : 0.0f;
    }

    double sampleRate = 48000.0;
    bool prepared = false;
    std::array<BandStages, 8> filters {};
};
} // namespace orchestral::dsp
