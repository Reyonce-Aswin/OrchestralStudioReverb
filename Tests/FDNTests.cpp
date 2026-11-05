#include "../Source/DSP/FDNReverb.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    using orchestral::dsp::FDNReverb;
    FDNReverb reverb;
    reverb.prepare(48000.0, 512);

    FDNReverb::Parameters p;
    p.decaySeconds = 2.0f;
    p.dampingHz = 10000.0f;
    p.inputGain = 0.25f;
    p.wetGain = 1.0f;
    reverb.setParameters(p);

    float peak = 0.0f;
    double totalEnergy = 0.0;
    double lateTailEnergy = 0.0;
    constexpr int sampleRate = 48000;
    constexpr int sampleCount = sampleRate * 8;

    for (int n = 0; n < sampleCount; ++n)
    {
        // A single impulse followed by seven seconds of silence models a
        // source note ending while the reverb network must keep ringing.
        const float in = (n == 0) ? 1.0f : 0.0f;
        float left = 0.0f, right = 0.0f;
        reverb.processSample(in, in, left, right);

        assert(std::isfinite(left) && std::isfinite(right));
        peak = std::max(peak, std::max(std::abs(left), std::abs(right)));
        const double sampleEnergy = static_cast<double>(left) * left
                                  + static_cast<double>(right) * right;
        totalEnergy += sampleEnergy;

        // Measure the final two seconds, well after the input has stopped.
        if (n >= sampleRate * 6)
            lateTailEnergy += sampleEnergy;
    }

    assert(peak > 0.0f);
    assert(totalEnergy > 0.0);
    assert(lateTailEnergy > 1.0e-12);
    std::cout << "FDN impulse/tail test passed. Peak=" << peak
              << ", total energy=" << totalEnergy
              << ", late-tail energy=" << lateTailEnergy << '\n';
    return 0;
}
