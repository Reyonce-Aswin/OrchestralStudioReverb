#include "../Source/DSP/FDNReverb.h"
#include "../Source/DSP/StagePosition.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

struct StereoEnergy
{
    double left = 0.0;
    double right = 0.0;
};

static StereoEnergy renderAtStage(int stage)
{
    using orchestral::dsp::FDNReverb;
    FDNReverb reverb;
    reverb.prepare(48000.0, 512);

    FDNReverb::Parameters p;
    p.decaySeconds = 2.0f;
    p.dampingHz = 10000.0f;
    p.inputGain = 0.25f;
    p.wetGain = 1.0f;
    p.stagePosition = stage;
    reverb.setParameters(p);

    StereoEnergy energy;
    constexpr int sampleCount = 48000 * 3;
    for (int n = 0; n < sampleCount; ++n)
    {
        const float input = (n == 0) ? 1.0f : 0.0f;
        float left = 0.0f;
        float right = 0.0f;
        reverb.processSample(input, input, left, right);
        assert(std::isfinite(left) && std::isfinite(right));
        energy.left += static_cast<double>(left) * left;
        energy.right += static_cast<double>(right) * right;
    }
    return energy;
}

static StereoEnergy renderAtPosition(orchestral::dsp::StagePosition position)
{
    using orchestral::dsp::FDNReverb;
    FDNReverb reverb;
    reverb.prepare(48000.0, 512);
    FDNReverb::Parameters p;
    p.decaySeconds = 2.0f;
    p.dampingHz = 10000.0f;
    p.inputGain = 0.25f;
    p.wetGain = 1.0f;
    p.useExplicitStagePosition = true;
    p.stagePan = position.pan;
    p.stageDepth = position.depth;
    reverb.setParameters(p);

    StereoEnergy energy;
    for (int n = 0; n < 48000 * 3; ++n)
    {
        const float input = n == 0 ? 1.0f : 0.0f;
        float left = 0.0f, right = 0.0f;
        reverb.processSample(input, input, left, right);
        energy.left += static_cast<double>(left) * left;
        energy.right += static_cast<double>(right) * right;
    }
    return energy;
}

int main()
{
    using orchestral::dsp::FDNReverb;

    // The same instrument must occupy different acoustic positions in
    // modern and antiphonal chamber seating.
    const auto chamberModernViolinI = orchestral::dsp::getStagePosition(
        orchestral::dsp::StageLayout::chamberModern, 1);
    const auto chamberClassicalViolinI = orchestral::dsp::getStagePosition(
        orchestral::dsp::StageLayout::chamberClassical, 1);
    const auto chamberClassicalViolinII = orchestral::dsp::getStagePosition(
        orchestral::dsp::StageLayout::chamberClassical, 2);
    assert(chamberModernViolinI.pan < -0.8f);
    assert(chamberClassicalViolinI.pan < -0.9f);
    assert(chamberClassicalViolinII.pan > 0.9f);
    assert(std::abs(chamberModernViolinI.pan - chamberClassicalViolinI.pan) > 0.01f);
    const auto chamberTimpani = orchestral::dsp::getStagePosition(
        orchestral::dsp::StageLayout::chamberModern, 13);
    assert(chamberTimpani.depth > chamberModernViolinI.depth);
    const auto modernRender = renderAtPosition(chamberModernViolinI);
    const auto antiphonalRender = renderAtPosition(chamberClassicalViolinI);
    const double spatialRenderDifference =
        std::abs(modernRender.left - antiphonalRender.left)
        + std::abs(modernRender.right - antiphonalRender.right);
    assert(spatialRenderDifference > 1.0e-8);
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
        const float in = (n == 0) ? 1.0f : 0.0f;
        float left = 0.0f, right = 0.0f;
        reverb.processSample(in, in, left, right);

        assert(std::isfinite(left) && std::isfinite(right));
        peak = std::max(peak, std::max(std::abs(left), std::abs(right)));
        const double sampleEnergy = static_cast<double>(left) * left
                                  + static_cast<double>(right) * right;
        totalEnergy += sampleEnergy;
        if (n >= sampleRate * 6)
            lateTailEnergy += sampleEnergy;
    }

    assert(peak > 0.0f);
    assert(totalEnergy > 0.0);
    assert(lateTailEnergy > 1.0e-12);

    // Different stage positions must create measurably different late-field
    // stereo perspectives, not just move the early-reflection taps.
    const auto leftStage = renderAtStage(1);
    const auto rightStage = renderAtStage(18);
    const double stageDifference =
        std::abs(leftStage.left - rightStage.left)
        + std::abs(leftStage.right - rightStage.right);
    assert(stageDifference > 1.0e-7);

    std::cout << "FDN impulse/tail and stage-position tests passed. Peak=" << peak
              << ", total energy=" << totalEnergy
              << ", late-tail energy=" << lateTailEnergy
              << ", stage difference=" << stageDifference << '\n';
    return 0;
}
