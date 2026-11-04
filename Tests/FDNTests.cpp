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
    p.wetGain = 0.5f;
    reverb.setParameters(p);

    float peak = 0.0f;
    double energy = 0.0;
    constexpr int sampleCount = 48000 * 8;
    for (int n = 0; n < sampleCount; ++n)
    {
        const float in = (n == 0) ? 1.0f : 0.0f;
        float left = 0.0f, right = 0.0f;
        reverb.processSample(in, in, left, right);
        assert(std::isfinite(left) && std::isfinite(right));
        peak = std::max(peak, std::max(std::abs(left), std::abs(right)));
        energy += static_cast<double>(left) * left + static_cast<double>(right) * right;
    }

    assert(peak > 0.0f);
    assert(energy > 0.0);
    std::cout << "FDN impulse test passed. Peak=" << peak
              << ", accumulated energy=" << energy << '\n';
    return 0;
}
