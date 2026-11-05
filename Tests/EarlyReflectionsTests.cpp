#include "../Source/DSP/EarlyReflections.h"

#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using orchestral::dsp::EarlyReflections;

    constexpr int sampleRate = 48000;
    EarlyReflections reflections;
    reflections.prepare(static_cast<double>(sampleRate));

    EarlyReflections::Parameters parameters;
    parameters.roomSize = 1.0f;
    parameters.preDelayMs = 12.0f;
    reflections.setParameters(parameters);

    int firstNonZeroSample = -1;
    double totalEnergy = 0.0;
    double lateReflectionEnergy = 0.0;

    for (int n = 0; n < sampleRate / 2; ++n)
    {
        const float input = (n == 0) ? 1.0f : 0.0f;
        float left = 0.0f;
        float right = 0.0f;
        reflections.processSample(input, input, left, right);

        assert(std::isfinite(left));
        assert(std::isfinite(right));

        const double energy = static_cast<double>(left) * left
                            + static_cast<double>(right) * right;
        totalEnergy += energy;

        if (firstNonZeroSample < 0 && energy > 1.0e-14)
            firstNonZeroSample = n;

        // The later taps should still be arriving after the first reflections.
        if (n > sampleRate / 20)
            lateReflectionEnergy += energy;
    }

    // A 12 ms pre-delay plus the first 8.1 ms tap should not appear immediately.
    assert(firstNonZeroSample > sampleRate / 100);
    assert(firstNonZeroSample < sampleRate / 10);
    assert(totalEnergy > 0.0);
    assert(lateReflectionEnergy > 1.0e-10);

    std::cout << "Early reflections impulse test passed. First reflection sample="
              << firstNonZeroSample << ", energy=" << totalEnergy
              << ", late reflection energy=" << lateReflectionEnergy << '\n';
    return 0;
}
