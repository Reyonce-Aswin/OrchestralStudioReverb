#include "ParametricEQ.h"
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
float measureRms(orchestral::dsp::ParametricEQ& eq, float frequency, double sampleRate)
{
    constexpr int count = 24000;
    double sum = 0.0;
    for (int i = 0; i < count; ++i)
    {
        const float input = std::sin(2.0f * 3.14159265358979323846f
                                     * frequency * static_cast<float>(i) / static_cast<float>(sampleRate));
        float left = input;
        float right = input;
        eq.processSample(left, right);
        if (i > count / 2)
            sum += static_cast<double>(left) * left;
    }
    return static_cast<float>(std::sqrt(sum / (count / 2)));
}
}

int main()
{
    constexpr double sampleRate = 48000.0;
    orchestral::dsp::ParametricEQ eq;
    eq.prepare(sampleRate);

    const float unityRms = measureRms(eq, 1000.0f, sampleRate);
    if (std::abs(unityRms - 0.7071f) > 0.01f)
    {
        std::cerr << "Disabled EQ is not transparent: " << unityRms << "\n";
        return 1;
    }

    orchestral::dsp::ParametricEQ::Band lowCut;
    lowCut.type = orchestral::dsp::ParametricEQ::Type::LowCut;
    lowCut.frequency = 1000.0f;
    lowCut.slope = 2;
    lowCut.enabled = true;
    eq.setBand(0, lowCut);
    const float attenuated = measureRms(eq, 100.0f, sampleRate);
    if (attenuated > 0.12f)
    {
        std::cerr << "Low-cut filter did not attenuate low frequencies enough: " << attenuated << "\n";
        return 2;
    }

    eq.reset();
    orchestral::dsp::ParametricEQ bellEQ;
    bellEQ.prepare(sampleRate);
    orchestral::dsp::ParametricEQ::Band bell;
    bell.type = orchestral::dsp::ParametricEQ::Type::Bell;
    bell.frequency = 1000.0f;
    bell.gainDb = 9.0f;
    bell.q = 1.0f;
    bell.enabled = true;
    bellEQ.setBand(3, bell);
    const float boosted = measureRms(bellEQ, 1000.0f, sampleRate);
    if (boosted < 1.7f)
    {
        std::cerr << "Bell filter did not apply the expected boost: " << boosted << "\n";
        return 3;
    }

    std::cout << "Parametric EQ tests passed\n";
    return 0;
}
