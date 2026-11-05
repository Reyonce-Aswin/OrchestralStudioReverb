#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <complex>
#include <utility>
#include <limits>

namespace
{
const juce::Colour background { 0x0F, 0x11, 0x1A };
const juce::Colour panel { 0x17, 0x1A, 0x26 };
const juce::Colour border { 0x3A, 0x3E, 0x4C };
const juce::Colour text { 0xEE, 0xF0, 0xF7 };
const juce::Colour muted { 0x8B, 0x90, 0xA3 };
const juce::Colour knobTrack { 0x3A, 0x3E, 0x4C };
const juce::Colour knobValue { 0xE8, 0xEA, 0xF0 };
const juce::Colour knobName { 0xD8, 0xDB, 0xE6 };
const juce::Colour sliderTrack { 0x2C, 0x2F, 0x3C };
const juce::Colour sliderFill { 0xC9, 0xCC, 0xD8 };
constexpr float kPi = 3.14159265358979323846f;

juce::Font uiFont(float height, bool bold = false)
{
    juce::FontOptions options;
    auto font = juce::Font(options.withName("Inter").withHeight(height)
                                .withStyle(bold ? "SemiBold" : "Regular"));
    if (!font.getTypefaceName().containsIgnoreCase("Inter"))
        font = juce::Font(options.withName("Arial").withHeight(height)
                              .withStyle(bold ? "Bold" : "Regular"));
    return font;
}

int getLayoutChoiceIndex(OrchestraLayoutComponent::Layout layout)
{
    switch (layout)
    {
        case OrchestraLayoutComponent::Layout::Standard: return 0;
        case OrchestraLayoutComponent::Layout::Classical: return 1;
        case OrchestraLayoutComponent::Layout::ChamberModern: return 2;
        case OrchestraLayoutComponent::Layout::ChamberClassical: return 3;
    }
    return 0;
}

int getStageChoiceIndex(const juce::StringArray& choices, juce::String section)
{
    section = section.trim();
    if (section.equalsIgnoreCase("VIOLIN I")) section = "Violins I";
    else if (section.equalsIgnoreCase("VIOLIN II")) section = "Violins II";
    else if (section.equalsIgnoreCase("HARPSICHORD / FORTEPIANO")) section = "Piano";
    else if (section.equalsIgnoreCase("PICCOLOS / FLUTES")) section = "Piccolos";
    else if (section.equalsIgnoreCase("TIMPANI")) section = "Percussion";

    for (int i = 0; i < choices.size(); ++i)
        if (choices[i].equalsIgnoreCase(section))
            return i;
    return -1;
}

void setChoiceParameter(juce::AudioProcessorValueTreeState& state,
                        const juce::String& parameterID, int choiceIndex)
{
    if (auto* parameter = state.getParameter(parameterID))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(choiceIndex)));
}

void drawLetterSpaced(juce::Graphics& g, const juce::String& value,
                      juce::Rectangle<float> area, float fontHeight, juce::Colour colour)
{
    const auto font = uiFont(fontHeight, true);
    float totalWidth = 0.0f;
    for (int i = 0; i < value.length(); ++i)
        totalWidth += static_cast<float>(font.getStringWidth(value.substring(i, i + 1))) + 1.5f;
    float x = area.getX() + (area.getWidth() - totalWidth) * 0.5f;
    g.setColour(colour);
    g.setFont(font);
    for (int i = 0; i < value.length(); ++i)
    {
        const auto character = value.substring(i, i + 1);
        const float width = static_cast<float>(font.getStringWidth(character));
        g.drawText(character, juce::Rectangle<float>(x, area.getY(), width + 1.0f, area.getHeight()),
                   juce::Justification::centred, false);
        x += width + 1.5f;
    }
}
}

void OSRControlLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float proportion, float startAngle, float endAngle,
                                              juce::Slider&)
{
    const float diameter = static_cast<float>(juce::jmin(width, height)) * 0.70f;
    const float radius = diameter * 0.5f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = startAngle + proportion * (endAngle - startAngle);
    // Build both arcs from the exact same angular convention as the pointer.
    // Sampling the sweep explicitly avoids JUCE arc-direction/angle-wrap
    // differences when the rotary range crosses 2*pi.
    const auto makeArc = [cx, cy, radius](float from, float to)
    {
        juce::Path path;
        constexpr int segments = 96;
        path.startNewSubPath(cx + std::cos(from) * radius,
                             cy + std::sin(from) * radius);
        for (int i = 1; i <= segments; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(segments);
            const float a = from + (to - from) * t;
            path.lineTo(cx + std::cos(a) * radius,
                        cy + std::sin(a) * radius);
        }
        return path;
    };

    const auto track = makeArc(startAngle, endAngle);
    const auto value = makeArc(startAngle, angle);
    const juce::PathStrokeType arcStroke(3.0f, juce::PathStrokeType::curved,
                                         juce::PathStrokeType::rounded);
    g.setColour(knobTrack);
    g.strokePath(track, arcStroke);
    g.setColour(knobValue);
    g.strokePath(value, arcStroke);

    // Same angle and coordinates as the circular value indicator.
    const juce::Point<float> pointer(std::cos(angle), std::sin(angle));
    g.setColour(juce::Colours::white);
    g.drawLine(cx + pointer.x * radius * 0.35f, cy + pointer.y * radius * 0.35f,
               cx + pointer.x * radius * 0.88f, cy + pointer.y * radius * 0.88f, 2.0f);
}

void OSRControlLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float, float,
                                              juce::Slider::SliderStyle style, juce::Slider&)
{
    if (style == juce::Slider::LinearHorizontal)
    {
        const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        const float left = static_cast<float>(x);
        const float right = static_cast<float>(x + width);
        g.setColour(sliderTrack);
        g.fillRoundedRectangle(left, cy - 2.0f, right - left, 4.0f, 2.0f);
        g.setColour(sliderFill);
        g.fillRoundedRectangle(left, cy - 2.0f, sliderPos - left, 4.0f, 2.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 5.0f, cy - 5.0f, 10.0f, 10.0f);
    }
    else
    {
        const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
        const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
        g.setColour(sliderTrack);
        g.fillRoundedRectangle(static_cast<float>(x), cy - 2.0f,
                               static_cast<float>(width), 4.0f, 2.0f);
        g.setColour(sliderFill);
        g.fillRoundedRectangle(static_cast<float>(x), cy - 2.0f,
                               juce::jlimit(0.0f, static_cast<float>(width), sliderPos - static_cast<float>(x)),
                               4.0f, 2.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        juce::ignoreUnused(cx);
    }
}

void OSRControlLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                                     bool, bool)
{
    g.setColour(button.findColour(button.getToggleState()
        ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId));
    g.setFont(uiFont(9.5f, true));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4),
                     juce::Justification::centred, 1);
}


float OSREQGraph::frequencyToX(float frequency) const
{
    const auto area = getLocalBounds().toFloat().reduced(48.0f, 26.0f);
    const float proportion = std::log(std::clamp(frequency, 20.0f, 20000.0f) / 20.0f)
                           / std::log(1000.0f);
    return area.getX() + proportion * area.getWidth();
}

float OSREQGraph::xToFrequency(float x) const
{
    const auto area = getLocalBounds().toFloat().reduced(48.0f, 26.0f);
    const float proportion = juce::jlimit(0.0f, 1.0f, (x - area.getX()) / area.getWidth());
    return 20.0f * std::pow(1000.0f, proportion);
}

float OSREQGraph::gainToY(float gain) const
{
    const auto area = getLocalBounds().toFloat().reduced(48.0f, 26.0f);
    return area.getY() + (18.0f - juce::jlimit(-18.0f, 18.0f, gain)) / 36.0f * area.getHeight();
}

float OSREQGraph::yToGain(float y) const
{
    const auto area = getLocalBounds().toFloat().reduced(48.0f, 26.0f);
    return juce::jlimit(-18.0f, 18.0f, 18.0f - 36.0f * (y - area.getY()) / area.getHeight());
}

void OSREQGraph::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour(panel);
    g.fillRoundedRectangle(area, 8.0f);
    g.setColour(border);
    g.drawRoundedRectangle(area, 8.0f, 1.0f);
    auto plot = area.reduced(48.0f, 26.0f);

    g.setFont(uiFont(9.0f));
    const float frequencies[] { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float frequency : frequencies)
    {
        const float x = frequencyToX(frequency);
        g.setColour(border.withAlpha(0.65f));
        g.drawVerticalLine(static_cast<int>(x), plot.getY(), plot.getBottom());
        const auto label = frequency >= 1000.0f
            ? juce::String(frequency / 1000.0f, frequency >= 10000.0f ? 0 : 1) + "k"
            : juce::String(static_cast<int>(frequency));
        g.setColour(muted);
        g.drawText(label, static_cast<int>(x - 22.0f), static_cast<int>(plot.getBottom() + 5.0f),
                   44, 14, juce::Justification::centred);
    }
    for (int db = -18; db <= 18; db += 6)
    {
        const float y = gainToY(static_cast<float>(db));
        g.setColour(db == 0 ? muted.withAlpha(0.8f) : border.withAlpha(0.55f));
        g.drawHorizontalLine(static_cast<int>(y), plot.getX(), plot.getRight());
        g.setColour(muted);
        g.drawText(juce::String(db), 5, static_cast<int>(y - 7.0f), 36, 14,
                   juce::Justification::centredRight);
    }

    auto& parameters = state();
    const auto value = [&parameters](const juce::String& id)
    {
        return parameters.getRawParameterValue(id)->load();
    };
    const bool eqBypassed = value("eqBypass") > 0.5f;
    const double reportedSampleRate = processor.getSampleRate();
    // The standalone preview can paint before an audio device is opened. In
    // that case JUCE reports 0 Hz; use a realistic nominal rate, never 8 kHz,
    // otherwise the 20 kHz graph wraps repeatedly above Nyquist.
    const double sr = reportedSampleRate >= 22050.0 ? reportedSampleRate : 48000.0;
    // Draw a conventional combined EQ response: multiply the magnitude of
    // every active filter section (add their dB responses). Use double precision
    // and oversample the display so narrow Q peaks cannot alias into false spikes.
    juce::Path response;
    const int responseSamples = juce::jmax(4096, static_cast<int>(plot.getWidth()) * 8);
    const auto sectionDb = [](double b0, double b1, double b2,
                              double a1, double a2, double hz, double sampleRate)
    {
        const double w = juce::MathConstants<double>::twoPi * hz / sampleRate;
        const double c1 = std::cos(w), s1 = std::sin(w);
        const double c2 = std::cos(2.0 * w), s2 = std::sin(2.0 * w);
        const double nr = b0 + b1 * c1 + b2 * c2;
        const double ni = -b1 * s1 - b2 * s2;
        const double dr = 1.0 + a1 * c1 + a2 * c2;
        const double di = -a1 * s1 - a2 * s2;
        const double numerator = nr * nr + ni * ni;
        const double denominator = dr * dr + di * di;
        if (!std::isfinite(numerator) || !std::isfinite(denominator) || denominator < 1.0e-24)
            return 0.0;
        return 10.0 * std::log10(std::max(1.0e-24, numerator / denominator));
    };

    for (int sample = 0; sample <= responseSamples; ++sample)
    {
        const double proportion = static_cast<double>(sample) / responseSamples;
        const double frequency = 20.0 * std::pow(1000.0, proportion);
        double totalDb = 0.0;

        if (!eqBypassed)
        {
            for (int band = 0; band < 8; ++band)
            {
                const auto prefix = "eqBand" + juce::String(band + 1);
                const bool enabled = value(prefix + "Enabled") > 0.5f;
                if (!enabled)
                    continue;

                const double centre = std::clamp(static_cast<double>(value(prefix + "Freq")),
                                                  20.0, sr * 0.45);
                const double gain = std::clamp(static_cast<double>(value(prefix + "Gain")),
                                                 -24.0, 24.0);
                const double q = std::max(0.3, static_cast<double>(value(prefix + "Q")));
                const int typeChoice = juce::roundToInt(value(prefix + "Type"));
                const bool lowCut = band == 0 && typeChoice == 0;
                const bool highCut = band == 7 && typeChoice == 1;
                const bool lowShelf = band == 0 && typeChoice == 1;
                const bool highShelf = band == 7 && typeChoice == 0;
                const bool isCut = lowCut || highCut;
                const int slope = juce::jlimit(0, 3, juce::roundToInt(value(prefix + "Slope")));
                const int biquadStages = isCut ? (slope == 0 ? 0 : (slope == 3 ? 2 : 1)) : 1;
                const bool firstOrder = isCut && (slope == 0 || slope == 2);
                const double w0 = juce::MathConstants<double>::twoPi * centre / sr;
                const double cosine = std::cos(w0);
                const double sine = std::sin(w0);
                const double amplitude = std::pow(10.0, gain / 40.0);
                const double resonance = std::clamp(q / 0.7071067811865476, 0.5, 2.5);
                constexpr double butterworth[4][4] = {
                    { 0.7071067811865476, 0.0, 0.0, 0.0 },
                    { 0.541196100146197, 1.306562964876377, 0.0, 0.0 },
                    { 0.517638090205042, 0.707106781186548, 1.931851652578136, 0.0 },
                    { 0.509795579104159, 0.601344886935045, 0.899976223136416, 2.562915447741506 }
                };

                const int sections = biquadStages + (firstOrder ? 1 : 0);
                for (int stage = 0; stage < sections; ++stage)
                {
                    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
                    double a0 = 1.0, a1 = 0.0, a2 = 0.0;

                    if (firstOrder && stage == biquadStages)
                    {
                        const double k = std::tan(w0 * 0.5);
                        const double norm = 1.0 / (1.0 + k);
                        if (lowCut) { b0 = norm; b1 = -norm; }
                        else { b0 = k * norm; b1 = k * norm; }
                        a1 = (k - 1.0) * norm;
                    }
                    else
                    {
                        const double stageQ = isCut
                            ? std::clamp(butterworth[std::max(0, biquadStages - 1)][stage] * resonance,
                                         0.25, 3.0)
                            : std::clamp(q, 0.1, 12.0);
                        const double alpha = sine / (2.0 * std::max(0.1, stageQ));

                        if (lowCut || highCut)
                        {
                            if (lowCut)
                            {
                                b0 = (1.0 + cosine) * 0.5;
                                b1 = -(1.0 + cosine);
                                b2 = b0;
                            }
                            else
                            {
                                b0 = (1.0 - cosine) * 0.5;
                                b1 = 1.0 - cosine;
                                b2 = b0;
                            }
                            a0 = 1.0 + alpha;
                            a1 = -2.0 * cosine;
                            a2 = 1.0 - alpha;
                        }
                        else if (!lowShelf && !highShelf)
                        {
                            b0 = 1.0 + alpha * amplitude;
                            b1 = -2.0 * cosine;
                            b2 = 1.0 - alpha * amplitude;
                            a0 = 1.0 + alpha / amplitude;
                            a1 = -2.0 * cosine;
                            a2 = 1.0 - alpha / amplitude;
                        }
                        else
                        {
                            const double shelfSlope = std::clamp(q / 0.7071067811865476, 0.1, 2.0);
                            const double shelfTerm = std::max(0.1,
                                (amplitude + 1.0 / amplitude) * (1.0 / shelfSlope - 1.0) + 2.0);
                            const double shelfAlpha = (sine * 0.5) * std::sqrt(shelfTerm);
                            const double root = 2.0 * std::sqrt(amplitude) * shelfAlpha;
                            if (lowShelf)
                            {
                                b0 = amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosine + root);
                                b1 = 2.0 * amplitude * ((amplitude - 1.0) - (amplitude + 1.0) * cosine);
                                b2 = amplitude * ((amplitude + 1.0) - (amplitude - 1.0) * cosine - root);
                                a0 = (amplitude + 1.0) + (amplitude - 1.0) * cosine + root;
                                a1 = -2.0 * ((amplitude - 1.0) + (amplitude + 1.0) * cosine);
                                a2 = (amplitude + 1.0) + (amplitude - 1.0) * cosine - root;
                            }
                            else
                            {
                                b0 = amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine + root);
                                b1 = -2.0 * amplitude * ((amplitude - 1.0) + (amplitude + 1.0) * cosine);
                                b2 = amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine - root);
                                a0 = (amplitude + 1.0) - (amplitude - 1.0) * cosine + root;
                                a1 = 2.0 * ((amplitude - 1.0) - (amplitude + 1.0) * cosine);
                                a2 = (amplitude + 1.0) - (amplitude - 1.0) * cosine - root;
                            }
                        }
                    }

                    const double inverseA0 = 1.0 / a0;
                    totalDb += sectionDb(b0 * inverseA0, b1 * inverseA0, b2 * inverseA0,
                                         a1 * inverseA0, a2 * inverseA0, frequency, sr);
                }
            }
        }

        const float x = plot.getX() + static_cast<float>(proportion) * plot.getWidth();
        const float y = gainToY(static_cast<float>(std::clamp(totalDb, -18.0, 18.0)));
        if (sample == 0) response.startNewSubPath(x, y);
        else response.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xD5, 0xD8, 0xE2));
    g.strokePath(response, juce::PathStrokeType(2.2f));

    // Explain why an inactive band can still shape the preview curve.
    g.setColour(muted);
    g.setFont(uiFont(9.0f));
    g.drawText(eqBypassed ? "EQ BYPASSED" : "RESPONSE PREVIEW",
               plot.getX() + 8.0f, plot.getY() + 6.0f, 140.0f, 14.0f,
               juce::Justification::left);

    for (int band = 0; band < 8; ++band)
    {
        const auto prefix = "eqBand" + juce::String(band + 1);
        const float frequency = value(prefix + "Freq");
        const float gain = value(prefix + "Gain");
        const float q = value(prefix + "Q");
        const int type = juce::roundToInt(value(prefix + "Type"));
        const bool cut = (band == 0 && type == 0) || (band == 7 && type == 1);
        const float x = frequencyToX(frequency);
        const float resonancePreview = juce::jlimit(-12.0f, 12.0f, (q - 0.7071f) * 12.0f);
        const float y = gainToY(cut ? resonancePreview : gain);
        const bool enabled = value(prefix + "Enabled") > 0.5f;
        g.setColour(enabled ? juce::Colour(0xC9, 0xCC, 0xD8) : muted);
        g.fillEllipse(x - (band == selectedBand ? 6.0f : 4.0f),
                      y - (band == selectedBand ? 6.0f : 4.0f),
                      band == selectedBand ? 12.0f : 8.0f,
                      band == selectedBand ? 12.0f : 8.0f);
        if (band == selectedBand)
        {
            g.setColour(text);
            g.drawEllipse(x - 9.0f, y - 9.0f, 18.0f, 18.0f, 1.0f);
        }
    }
}

void OSREQGraph::mouseDown(const juce::MouseEvent& event)
{
    int nearest = 0;
    float nearestDistance = std::numeric_limits<float>::max();
    for (int band = 0; band < 8; ++band)
    {
        const auto prefix = "eqBand" + juce::String(band + 1);
        const float frequency = state().getRawParameterValue(prefix + "Freq")->load();
        const float x = frequencyToX(frequency);
        const float gain = state().getRawParameterValue(prefix + "Gain")->load();
        const float q = state().getRawParameterValue(prefix + "Q")->load();
        const int type = juce::roundToInt(state().getRawParameterValue(prefix + "Type")->load());
        const bool cut = (band == 0 && type == 0) || (band == 7 && type == 1);
        const float resonancePreview = juce::jlimit(-12.0f, 12.0f, (q - 0.7071f) * 12.0f);
        const float y = gainToY(cut ? resonancePreview : gain);
        const float distance = std::hypot(event.position.x - x, event.position.y - y);
        if (distance < nearestDistance) { nearestDistance = distance; nearest = band; }
    }
    selectedBand = nearest;
    if (onBandSelected) onBandSelected(selectedBand);
    mouseDrag(event);
    repaint();
}

void OSREQGraph::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    int nearest = -1;
    float nearestDistance = 20.0f;
    for (int band = 0; band < 8; ++band)
    {
        const auto prefix = "eqBand" + juce::String(band + 1);
        const float frequency = state().getRawParameterValue(prefix + "Freq")->load();
        const float gain = state().getRawParameterValue(prefix + "Gain")->load();
        const float q = state().getRawParameterValue(prefix + "Q")->load();
        const int type = juce::roundToInt(state().getRawParameterValue(prefix + "Type")->load());
        const bool cut = (band == 0 && type == 0) || (band == 7 && type == 1);
        const float y = gainToY(cut ? juce::jlimit(-12.0f, 12.0f, (q - 0.7071f) * 12.0f) : gain);
        const float distance = std::hypot(event.position.x - frequencyToX(frequency), event.position.y - y);
        if (distance < nearestDistance) { nearestDistance = distance; nearest = band; }
    }
    if (nearest < 0 || std::abs(wheel.deltaY) < 0.001f)
        return;

    selectedBand = nearest;
    if (onBandSelected) onBandSelected(selectedBand);
    const auto parameterID = "eqBand" + juce::String(selectedBand + 1) + "Q";
    if (auto* parameter = state().getParameter(parameterID))
    {
        const float current = state().getRawParameterValue(parameterID)->load();
        const float direction = wheel.deltaY > 0.0f ? 1.0f : -1.0f;
        const float step = juce::jmax(0.01f, current * 0.06f);
        const float next = juce::jlimit(0.3f, selectedBand == 0 || selectedBand == 7 ? 2.0f : 8.0f,
                                        current + direction * step * juce::jmax(1.0f, std::abs(wheel.deltaY) * 4.0f));
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(next));
        parameter->endChangeGesture();
    }
    repaint();
}

void OSREQGraph::mouseDrag(const juce::MouseEvent& event)
{
    const auto prefix = "eqBand" + juce::String(selectedBand + 1);
    const float frequency = xToFrequency(event.position.x);
    if (auto* parameter = state().getParameter(prefix + "Freq"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(frequency));
        parameter->endChangeGesture();
    }
    const int type = juce::roundToInt(state().getRawParameterValue(prefix + "Type")->load());
    const bool cut = (selectedBand == 0 && type == 0) || (selectedBand == 7 && type == 1);
    const auto verticalValue = yToGain(event.position.y);
    const auto parameterID = cut ? prefix + "Q" : prefix + "Gain";
    if (auto* parameter = state().getParameter(parameterID))
    {
        const float value = cut
            ? juce::jlimit(0.3f, 2.0f, 0.7071f + verticalValue / 12.0f)
            : verticalValue;
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        parameter->endChangeGesture();
    }
    repaint();
}

OrchestralStudioReverbAudioProcessorEditor::OrchestralStudioReverbAudioProcessorEditor(
    OrchestralStudioReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), eqGraph(p)
{
    title.setText("ORCHESTRAL STUDIO REVERB", juce::dontSendNotification);
    title.setFont(uiFont(18.0f, true));
    title.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(title);

    presetLabel.setText("CONCERT HALL", juce::dontSendNotification);
    presetLabel.setFont(uiFont(12.0f));
    presetLabel.setColour(juce::Label::textColourId, text);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    selectorTitle.setFont(uiFont(10.0f, true));
    selectorTitle.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(selectorTitle);

    stagePositionLabel.setFont(uiFont(10.0f));
    stagePositionLabel.setColour(juce::Label::textColourId, muted);
    stagePositionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(stagePositionLabel);
    addAndMakeVisible(stageMap);

    auto setupLayoutButton = [this](juce::TextButton& button)
    {
        button.setClickingTogglesState(false);
        button.setColour(juce::TextButton::buttonColourId, panel);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x25, 0x34, 0x5E));
        button.setColour(juce::TextButton::textColourOffId, text);
        button.setColour(juce::TextButton::textColourOnId, text);
        button.setLookAndFeel(&controlLookAndFeel);
        addAndMakeVisible(button);
    };
    setupLayoutButton(standardLayoutButton);
    setupLayoutButton(classicalLayoutButton);
    setupLayoutButton(chamberModernButton);
    setupLayoutButton(chamberClassicalButton);

    auto changeLayout = [this](OrchestraLayoutComponent::Layout layout)
    {
        stageMap.setLayout(layout);
        lastDisplayedStageSelection = stageMap.getSelectedSection();
        setChoiceParameter(processor.getParameters(), "stageLayout", getLayoutChoiceIndex(layout));
        const auto& choices = dynamic_cast<juce::AudioParameterChoice*>(
            processor.getParameters().getParameter("stagePosition"))->choices;
        const int stageIndex = getStageChoiceIndex(choices, stageMap.getSelectedParameterName());
        if (stageIndex >= 0)
            setChoiceParameter(processor.getParameters(), "stagePosition", stageIndex);
        selectorTitle.setText(stageMap.getTitle(), juce::dontSendNotification);
        stagePositionLabel.setText(juce::String{}, juce::dontSendNotification);
        updateLayoutButtons();
    };
    standardLayoutButton.onClick = [changeLayout] { changeLayout(OrchestraLayoutComponent::Layout::Standard); };
    classicalLayoutButton.onClick = [changeLayout] { changeLayout(OrchestraLayoutComponent::Layout::Classical); };
    chamberModernButton.onClick = [changeLayout] { changeLayout(OrchestraLayoutComponent::Layout::ChamberModern); };
    chamberClassicalButton.onClick = [changeLayout] { changeLayout(OrchestraLayoutComponent::Layout::ChamberClassical); };

    const auto setupSlider = [this](juce::Slider& slider, juce::Label& label,
                                    juce::Label& valueLabel, const juce::String& name)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters(juce::degreesToRadians(135.0f),
                                   juce::degreesToRadians(405.0f), true);
        slider.setLookAndFeel(&controlLookAndFeel);
        slider.setColour(juce::Slider::rotarySliderFillColourId, knobValue);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, knobTrack);
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);

        label.setText(name, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, knobName);
        label.setFont(uiFont(9.5f));

        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setColour(juce::Label::textColourId, muted);
        valueLabel.setFont(uiFont(11.0f));

        addAndMakeVisible(slider);
        addAndMakeVisible(label);
        addAndMakeVisible(valueLabel);
    };

    setupSlider(roomSize, roomSizeLabel, roomSizeValue, "ROOM SIZE");
    setupSlider(preDelay, preDelayLabel, preDelayValue, "PRE-DELAY");
    setupSlider(decay, decayLabel, decayValue, "DECAY");
    setupSlider(damping, dampingLabel, dampingValue, "DAMPING");
    setupSlider(early, earlyLabel, earlyValue, "EARLY LEVEL");
    setupSlider(dry, dryLabel, dryValue, "DRY");
    setupSlider(wet, wetLabel, wetValue, "LATE WET");

    auto& state = processor.getParameters();
    decayAttachment = std::make_unique<Attachment>(state, "decay", decay);
    dampingAttachment = std::make_unique<Attachment>(state, "damping", damping);
    roomSizeAttachment = std::make_unique<Attachment>(state, "roomSize", roomSize);
    preDelayAttachment = std::make_unique<Attachment>(state, "preDelay", preDelay);
    earlyAttachment = std::make_unique<Attachment>(state, "early", early);
    dryAttachment = std::make_unique<Attachment>(state, "dry", dry);
    wetAttachment = std::make_unique<Attachment>(state, "wet", wet);

    const auto refreshValues = [this] { updateValueLabels(); };
    roomSize.onValueChange = refreshValues;
    preDelay.onValueChange = refreshValues;
    decay.onValueChange = refreshValues;
    damping.onValueChange = refreshValues;
    early.onValueChange = refreshValues;
    dry.onValueChange = refreshValues;
    wet.onValueChange = refreshValues;
    updateValueLabels();


    auto setupTabButton = [this](juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, panel);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x25, 0x34, 0x5E));
        button.setColour(juce::TextButton::textColourOffId, text);
        button.setColour(juce::TextButton::textColourOnId, text);
        button.setLookAndFeel(&controlLookAndFeel);
        button.setClickingTogglesState(false);
        addAndMakeVisible(button);
    };
    setupTabButton(stageViewButton);
    setupTabButton(eqViewButton);
    setupTabButton(eqBypassButton);
    stageViewButton.onClick = [this] { setActiveView(false); };
    eqViewButton.onClick = [this] { setActiveView(true); };
    eqTypeBox.onChange = [this] { updateEQBandTypeControls(); eqGraph.repaint(); };
    eqBypassButton.setClickingTogglesState(true);
    eqBypassAttachment = std::make_unique<ButtonAttachment>(state, "eqBypass", eqBypassButton);

    addAndMakeVisible(eqGraph);
    eqGraph.onBandSelected = [this](int band) { selectEQBand(band); };
    for (int band = 0; band < 8; ++band)
    {
        auto& button = eqBandButtons[band];
        button.setButtonText(juce::String(band + 1));
        button.setColour(juce::TextButton::buttonColourId, panel);
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x25, 0x34, 0x5E));
        button.setColour(juce::TextButton::textColourOffId, text);
        button.setColour(juce::TextButton::textColourOnId, text);
        button.setLookAndFeel(&controlLookAndFeel);
        button.setClickingTogglesState(false);
        button.onClick = [this, band] { selectEQBand(band); };
        addAndMakeVisible(button);
    }
    eqEnableButton.setClickingTogglesState(true);
    eqEnableButton.setColour(juce::TextButton::buttonColourId, panel);
    eqEnableButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x25, 0x34, 0x5E));
    eqEnableButton.setColour(juce::TextButton::textColourOffId, text);
    eqEnableButton.setColour(juce::TextButton::textColourOnId, text);
    eqEnableButton.setLookAndFeel(&controlLookAndFeel);
    addAndMakeVisible(eqEnableButton);

    const auto setupEQSlider = [this](juce::Slider& slider, juce::Label& label,
                                      const juce::String& name, double minimum, double maximum,
                                      double interval, double skew)
    {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setRange(minimum, maximum, interval);
        if (skew > 0.0) slider.setSkewFactor(skew);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&controlLookAndFeel);
        slider.setColour(juce::Slider::textBoxTextColourId, text);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, panel);
        slider.setColour(juce::Slider::textBoxOutlineColourId, border);
        label.setText(name, juce::dontSendNotification);
        label.setFont(uiFont(9.5f, true));
        label.setColour(juce::Label::textColourId, muted);
        addAndMakeVisible(slider);
        addAndMakeVisible(label);
    };
    setupEQSlider(eqFrequency, eqFrequencyLabel, "FREQUENCY", 20.0, 20000.0, 0.1, 0.25);
    setupEQSlider(eqGain, eqGainLabel, "GAIN", -18.0, 18.0, 0.1, 0.0);
    setupEQSlider(eqQ, eqQLabel, "RESONANCE / Q", 0.3, 8.0, 0.01, 0.5);
    setupEQSlider(eqMix, eqMixLabel, "EQ MIX", 0.0, 1.0, 0.001, 0.0);
    for (auto* label : { &eqFrequencyValue, &eqGainValue, &eqQValue, &eqMixValue })
    {
        label->setFont(uiFont(11.0f));
        label->setColour(juce::Label::textColourId, muted);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*label);
    }
    eqMixAttachment = std::make_unique<Attachment>(state, "eqMix", eqMix);
    eqMix.onValueChange = [this] { updateEQValueLabels(); };

    eqTypeBox.setColour(juce::ComboBox::backgroundColourId, panel);
    eqTypeBox.setColour(juce::ComboBox::textColourId, text);
    eqTypeBox.setColour(juce::ComboBox::outlineColourId, border);
    eqSlopeBox.setColour(juce::ComboBox::backgroundColourId, panel);
    eqSlopeBox.setColour(juce::ComboBox::textColourId, text);
    eqSlopeBox.setColour(juce::ComboBox::outlineColourId, border);
    eqTypeLabel.setText("FILTER", juce::dontSendNotification);
    eqSlopeLabel.setText("SLOPE", juce::dontSendNotification);
    for (auto* label : { &eqTypeLabel, &eqSlopeLabel })
    {
        label->setFont(uiFont(9.5f, true));
        label->setColour(juce::Label::textColourId, muted);
        addAndMakeVisible(*label);
    }
    addAndMakeVisible(eqTypeBox);
    addAndMakeVisible(eqSlopeBox);
    selectEQBand(0);
    setActiveView(false);

    stageMap.onSelectionChanged = [this](const juce::String& sectionName)
    {
        stagePositionLabel.setText("SELECTED: " + sectionName.toUpperCase(), juce::dontSendNotification);
        lastDisplayedStageSelection = stageMap.getSelectedSection();
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*>(
                processor.getParameters().getParameter("stagePosition")))
        {
            const int index = getStageChoiceIndex(choice->choices, sectionName);
            if (index >= 0)
                setChoiceParameter(processor.getParameters(), "stagePosition", index);
        }
    };
    stageMap.onLayoutChanged = [this](OrchestraLayoutComponent::Layout) { updateLayoutButtons(); };

    const int savedLayout = juce::jlimit(0, 3, juce::roundToInt(
        processor.getParameters().getRawParameterValue("stageLayout")->load()));
    stageMap.setLayout(static_cast<OrchestraLayoutComponent::Layout>(savedLayout));
    lastDisplayedStageSelection = stageMap.getSelectedSection();
    selectorTitle.setText(stageMap.getTitle(), juce::dontSendNotification);
    syncStageSelectionFromHost();
    updateLayoutButtons();
    startTimerHz(20);
    setResizable(true, true);
    setResizeLimits(850, 620, (std::numeric_limits<int>::max)(),
                    (std::numeric_limits<int>::max)());
    setSize(1000, 700);
}

OrchestralStudioReverbAudioProcessorEditor::~OrchestralStudioReverbAudioProcessorEditor()
{
    for (auto* slider : { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet })
        slider->setLookAndFeel(nullptr);
    standardLayoutButton.setLookAndFeel(nullptr);
    classicalLayoutButton.setLookAndFeel(nullptr);
    chamberModernButton.setLookAndFeel(nullptr);
    chamberClassicalButton.setLookAndFeel(nullptr);

    stageViewButton.setLookAndFeel(nullptr);
    eqViewButton.setLookAndFeel(nullptr);
    eqBypassButton.setLookAndFeel(nullptr);
    eqEnableButton.setLookAndFeel(nullptr);
    for (auto& button : eqBandButtons) button.setLookAndFeel(nullptr);
    eqFrequency.setLookAndFeel(nullptr);
    eqGain.setLookAndFeel(nullptr);
    eqQ.setLookAndFeel(nullptr);
    eqMix.setLookAndFeel(nullptr);
}


void OrchestralStudioReverbAudioProcessorEditor::selectEQBand(int band)
{
    selectedEQBand = juce::jlimit(0, 7, band);
    eqGraph.setSelectedBand(selectedEQBand);
    for (int i = 0; i < 8; ++i)
        eqBandButtons[i].setColour(juce::TextButton::buttonColourId,
            i == selectedEQBand ? juce::Colour(0x25, 0x34, 0x5E) : panel);
    rebuildEQAttachments();
}

void OrchestralStudioReverbAudioProcessorEditor::rebuildEQAttachments()
{
    eqFrequencyAttachment.reset();
    eqGainAttachment.reset();
    eqQAttachment.reset();
    eqEnableAttachment.reset();
    eqTypeAttachment.reset();
    eqSlopeAttachment.reset();

    auto& state = processor.getParameters();
    const auto prefix = "eqBand" + juce::String(selectedEQBand + 1);
    eqQ.setRange(0.3, (selectedEQBand == 0 || selectedEQBand == 7) ? 2.0 : 8.0, 0.01);
    eqFrequencyAttachment = std::make_unique<Attachment>(state, prefix + "Freq", eqFrequency);
    eqGainAttachment = std::make_unique<Attachment>(state, prefix + "Gain", eqGain);
    eqQAttachment = std::make_unique<Attachment>(state, prefix + "Q", eqQ);
    eqFrequency.onValueChange = [this] { updateEQValueLabels(); };
    eqGain.onValueChange = [this] { updateEQValueLabels(); };
    eqQ.onValueChange = [this] { updateEQValueLabels(); };
    updateEQValueLabels();
    eqEnableAttachment = std::make_unique<ButtonAttachment>(state, prefix + "Enabled", eqEnableButton);

    eqTypeBox.clear(juce::dontSendNotification);
    if (selectedEQBand == 0)
    {
        eqTypeBox.addItem("LOW CUT", 1);
        eqTypeBox.addItem("LOW SHELF", 2);
    }
    else if (selectedEQBand == 7)
    {
        eqTypeBox.addItem("HIGH SHELF", 1);
        eqTypeBox.addItem("HIGH CUT", 2);
    }
    else
        eqTypeBox.addItem("BELL", 1);

    eqSlopeBox.clear(juce::dontSendNotification);
    eqSlopeBox.addItem("6 dB / OCT", 1);
    eqSlopeBox.addItem("12 dB / OCT", 2);
    eqSlopeBox.addItem("18 dB / OCT", 3);
    eqSlopeBox.addItem("24 dB / OCT", 4);
    eqTypeAttachment = std::make_unique<ComboAttachment>(state, prefix + "Type", eqTypeBox);
    eqSlopeAttachment = std::make_unique<ComboAttachment>(state, prefix + "Slope", eqSlopeBox);
    updateEQBandTypeControls();
    eqGraph.repaint();
}

void OrchestralStudioReverbAudioProcessorEditor::updateEQBandTypeControls()
{
    auto& state = processor.getParameters();
    const auto prefix = "eqBand" + juce::String(selectedEQBand + 1);
    const int type = juce::roundToInt(state.getRawParameterValue(prefix + "Type")->load());
    const bool cut = (selectedEQBand == 0 && type == 0) || (selectedEQBand == 7 && type == 1);
    const bool shelf = (selectedEQBand == 0 && type == 1) || (selectedEQBand == 7 && type == 0);
    eqGain.setEnabled(!cut);
    eqGainLabel.setText(cut ? "GAIN (N/A FOR CUT)" : "GAIN", juce::dontSendNotification);
    eqQLabel.setText(cut ? "RESONANCE" : (shelf ? "SHELF SLOPE" : "Q"),
                     juce::dontSendNotification);
    eqTypeBox.setVisible(showingEQ && (selectedEQBand == 0 || selectedEQBand == 7));
    eqTypeLabel.setVisible(showingEQ && (selectedEQBand == 0 || selectedEQBand == 7));
    eqSlopeBox.setVisible(showingEQ && cut);
    eqSlopeLabel.setVisible(showingEQ && cut);
}

void OrchestralStudioReverbAudioProcessorEditor::updateEQValueLabels()
{
    const double frequency = eqFrequency.getValue();
    eqFrequencyValue.setText(frequency >= 1000.0
        ? juce::String(frequency / 1000.0, 2) + " kHz"
        : juce::String(frequency, 1) + " Hz", juce::dontSendNotification);
    eqGainValue.setText(juce::String(eqGain.getValue(), 1) + " dB", juce::dontSendNotification);
    eqQValue.setText(juce::String(eqQ.getValue(), 2), juce::dontSendNotification);
    eqMixValue.setText(juce::String(eqMix.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
}

void OrchestralStudioReverbAudioProcessorEditor::setActiveView(bool showEQ)
{
    showingEQ = showEQ;
    rebuildEQAttachments();
    const bool showStage = !showEQ;
    selectorTitle.setVisible(showStage);
    stagePositionLabel.setVisible(showStage);
    stageMap.setVisible(showStage);
    standardLayoutButton.setVisible(showStage);
    classicalLayoutButton.setVisible(showStage);
    chamberModernButton.setVisible(showStage);
    chamberClassicalButton.setVisible(showStage);
    for (auto* slider : { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet })
        slider->setVisible(showStage);
    for (auto* label : { &roomSizeLabel, &preDelayLabel, &decayLabel, &dampingLabel,
                         &earlyLabel, &dryLabel, &wetLabel, &roomSizeValue, &preDelayValue,
                         &decayValue, &dampingValue, &earlyValue, &dryValue, &wetValue })
        label->setVisible(showStage);

    eqGraph.setVisible(showEQ);
    eqBypassButton.setVisible(showEQ);
    eqEnableButton.setVisible(showEQ);
    eqFrequency.setVisible(showEQ);
    eqGain.setVisible(showEQ);
    eqQ.setVisible(showEQ);
    eqMix.setVisible(showEQ);
    eqFrequencyLabel.setVisible(showEQ);
    eqGainLabel.setVisible(showEQ);
    eqQLabel.setVisible(showEQ);
    eqMixLabel.setVisible(showEQ);
    eqFrequencyValue.setVisible(showEQ);
    eqGainValue.setVisible(showEQ);
    eqQValue.setVisible(showEQ);
    eqMixValue.setVisible(showEQ);
    eqFrequencyLabel.setVisible(showEQ);
    eqGainLabel.setVisible(showEQ);
    eqQLabel.setVisible(showEQ);
    eqTypeBox.setVisible(showEQ && (selectedEQBand == 0 || selectedEQBand == 7));
    eqTypeLabel.setVisible(showEQ && (selectedEQBand == 0 || selectedEQBand == 7));
    eqSlopeBox.setVisible(showEQ && eqSlopeBox.isVisible());
    eqSlopeLabel.setVisible(showEQ && eqSlopeLabel.isVisible());
    for (auto& button : eqBandButtons) button.setVisible(showEQ);

    stageViewButton.setColour(juce::TextButton::buttonColourId, showStage
        ? juce::Colour(0x25, 0x34, 0x5E) : panel);
    eqViewButton.setColour(juce::TextButton::buttonColourId, showEQ
        ? juce::Colour(0x25, 0x34, 0x5E) : panel);
    resized();
    repaint();
}

void OrchestralStudioReverbAudioProcessorEditor::updateLayoutButtons()
{
    const auto active = juce::Colour(0x25, 0x34, 0x5E);
    const auto layout = stageMap.getLayout();
    const std::pair<juce::TextButton*, OrchestraLayoutComponent::Layout> buttons[] = {
        { &standardLayoutButton, OrchestraLayoutComponent::Layout::Standard },
        { &classicalLayoutButton, OrchestraLayoutComponent::Layout::Classical },
        { &chamberModernButton, OrchestraLayoutComponent::Layout::ChamberModern },
        { &chamberClassicalButton, OrchestraLayoutComponent::Layout::ChamberClassical }
    };
    for (const auto& entry : buttons)
    {
        entry.first->setColour(juce::TextButton::buttonColourId,
                               layout == entry.second ? active : panel);
        entry.first->repaint();
    }
}

void OrchestralStudioReverbAudioProcessorEditor::updateValueLabels()
{
    roomSizeValue.setText(juce::String(roomSize.getValue(), 2) + " "
                              + juce::String::charToString(0x00D7),
                          juce::dontSendNotification);
    preDelayValue.setText(juce::String(preDelay.getValue(), 1) + " ms", juce::dontSendNotification);
    decayValue.setText(juce::String(decay.getValue(), 2) + " s", juce::dontSendNotification);

    const double dampingHz = damping.getValue();
    dampingValue.setText(dampingHz >= 1000.0
        ? juce::String(dampingHz / 1000.0, 2) + " kHz"
        : juce::String(dampingHz, 0) + " Hz", juce::dontSendNotification);

    earlyValue.setText(juce::String(early.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
    dryValue.setText(juce::String(dry.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
    wetValue.setText(juce::String(wet.getValue() * 100.0, 1) + "%", juce::dontSendNotification);
}

void OrchestralStudioReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    const int controlsTop = getHeight() - 166;
    g.setColour(border);
    g.drawLine(24.0f, 64.0f, static_cast<float>(getWidth() - 24), 64.0f, 1.0f);
    if (!showingEQ)
    {
        g.drawLine(24.0f, static_cast<float>(controlsTop - 10),
                   static_cast<float>(getWidth() - 24), static_cast<float>(controlsTop - 10), 1.0f);
        drawLetterSpaced(g, "ROOM / REVERB / MIX", { 26.0f, static_cast<float>(controlsTop - 7),
                                                      220.0f, 15.0f }, 9.0f, muted);
    }
}

void OrchestralStudioReverbAudioProcessorEditor::resized()
{
    constexpr int margin = 26;
    title.setBounds(margin, 14, 360, 34);
    presetLabel.setBounds(getWidth() / 2 - 100, 14, 200, 34);
    stageViewButton.setBounds(getWidth() - 178, 17, 72, 28);
    eqViewButton.setBounds(getWidth() - 98, 17, 72, 28);

    if (showingEQ)
    {
        eqBypassButton.setBounds(26, 72, 122, 25);
        // Universal wet-path EQ blend, independent of the selected band.
        const int mixGroupWidth = 330;
        const int mixGroupX = getWidth() - 26 - mixGroupWidth;
        eqMixLabel.setBounds(mixGroupX, 77, 52, 14);
        eqMix.setBounds(mixGroupX + 54, 68, 214, 34);
        eqMixValue.setBounds(mixGroupX + 270, 76, 60, 18);
        eqGraph.setBounds(26, 104, getWidth() - 52, 330);
        const int bandWidth = (getWidth() - 52) / 8;
        for (int i = 0; i < 8; ++i)
            eqBandButtons[i].setBounds(26 + i * bandWidth, 444, bandWidth - 5, 30);
        eqEnableButton.setBounds(26, 492, 96, 28);
        eqTypeLabel.setBounds(140, 488, 120, 14);
        eqTypeBox.setBounds(140, 504, 120, 28);
        eqSlopeLabel.setBounds(276, 488, 140, 14);
        eqSlopeBox.setBounds(276, 504, 140, 28);
        const int columnWidth = (getWidth() - 52) / 3;
        eqFrequencyLabel.setBounds(26, 545, columnWidth - 12, 14);
        eqFrequency.setBounds(26, 560, columnWidth - 12, 50);
        eqGainLabel.setBounds(26 + columnWidth, 545, columnWidth - 12, 14);
        eqGain.setBounds(26 + columnWidth, 560, columnWidth - 12, 50);
        eqQLabel.setBounds(26 + columnWidth * 2, 545, columnWidth - 12, 14);
        eqQ.setBounds(26 + columnWidth * 2, 560, columnWidth - 12, 34);
        eqFrequencyValue.setBounds(26, 596, columnWidth - 12, 18);
        eqGainValue.setBounds(26 + columnWidth, 596, columnWidth - 12, 18);
        eqQValue.setBounds(26 + columnWidth * 2, 596, columnWidth - 12, 18);
        return;
    }

    selectorTitle.setBounds(26, 69, 220, 27);
    standardLayoutButton.setBounds(250, 69, 112, 27);
    classicalLayoutButton.setBounds(368, 69, 126, 27);
    chamberModernButton.setBounds(500, 69, 132, 27);
    chamberClassicalButton.setBounds(638, 69, 150, 27);
    stagePositionLabel.setBounds(794, 69, getWidth() - 820, 27);

    const int controlsTop = getHeight() - 166;
    const int mapTop = 106;
    const int mapHeight = juce::jmax(220, controlsTop - mapTop - 14);
    stageMap.setBounds(0, mapTop, getWidth(), mapHeight);

    const int count = 7;
    const int availableWidth = getWidth() - margin * 2;
    const int cellWidth = availableWidth / count;
    juce::Slider* sliders[] = { &roomSize, &preDelay, &decay, &damping, &early, &dry, &wet };
    juce::Label* labels[] = { &roomSizeLabel, &preDelayLabel, &decayLabel, &dampingLabel,
                              &earlyLabel, &dryLabel, &wetLabel };
    juce::Label* values[] = { &roomSizeValue, &preDelayValue, &decayValue, &dampingValue,
                              &earlyValue, &dryValue, &wetValue };
    for (int i = 0; i < count; ++i)
    {
        const int x = margin + i * cellWidth;
        labels[i]->setBounds(x, controlsTop + 3, cellWidth, 15);
        const int dialWidth = juce::jmin(88, cellWidth - 8);
        sliders[i]->setBounds(x + (cellWidth - dialWidth) / 2, controlsTop + 19, dialWidth, 88);
        values[i]->setBounds(x, controlsTop + 107, cellWidth, 20);
    }
}

void OrchestralStudioReverbAudioProcessorEditor::syncStageSelectionFromHost()
{
    const int layoutIndex = juce::jlimit(0, 3, juce::roundToInt(
        processor.getParameters().getRawParameterValue("stageLayout")->load()));
    const auto layout = static_cast<OrchestraLayoutComponent::Layout>(layoutIndex);
    if (stageMap.getLayout() != layout)
    {
        stageMap.setLayout(layout);
        lastDisplayedStageSelection = stageMap.getSelectedSection();
        selectorTitle.setText(stageMap.getTitle(), juce::dontSendNotification);
        updateLayoutButtons();
    }

    if (auto* parameter = processor.getParameters().getParameter("stagePosition"))
    {
        const auto name = parameter->getCurrentValueAsText();
        const auto currentChoice = getStageChoiceIndex(
            dynamic_cast<juce::AudioParameterChoice*>(parameter)->choices,
            stageMap.getSelectedParameterName());
        const auto hostChoice = getStageChoiceIndex(
            dynamic_cast<juce::AudioParameterChoice*>(parameter)->choices, name);
        if (hostChoice >= 0 && currentChoice != hostChoice)
            stageMap.setSelectedSection(name);
        const auto selected = stageMap.getSelectedSection();
        if (selected != lastDisplayedStageSelection)
        {
            lastDisplayedStageSelection = selected;
            stagePositionLabel.setText("SELECTED: " + selected.toUpperCase(),
                                       juce::dontSendNotification);
        }
    }
}

void OrchestralStudioReverbAudioProcessorEditor::timerCallback()
{
    syncStageSelectionFromHost();
    updateEQBandTypeControls();
    const auto prefix = "eqBand" + juce::String(selectedEQBand + 1);
    const bool bandEnabled = processor.getParameters().getRawParameterValue(prefix + "Enabled")->load() > 0.5f;
    eqEnableButton.setButtonText(bandEnabled ? "BAND ON" : "BAND OFF");
    updateEQValueLabels();
    eqGraph.repaint();
}
