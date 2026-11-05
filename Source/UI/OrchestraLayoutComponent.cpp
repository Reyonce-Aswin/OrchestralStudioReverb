#include "OrchestraLayoutComponent.h"

#include <cmath>

namespace
{
const juce::Colour backgroundColour { 0x0F, 0x11, 0x1A };
const juce::Colour fillColour       { 0x17, 0x1A, 0x26 };
const juce::Colour hoverColour      { 0x22, 0x28, 0x42 };
const juce::Colour selectedColour   { 0x25, 0x34, 0x5E };
const juce::Colour outlineColour    { 0xE6, 0xE9, 0xF2 };
const juce::Colour selectedOutline  { 0x8D, 0xB4, 0xFF };
const juce::Colour labelColour      { 0xEE, 0xF0, 0xF7 };
constexpr float kPi = 3.14159265358979323846f;
constexpr float kGapPixels = 5.0f;
constexpr float kDesignUnitsAt1200 = 750.0f;

float toRadians(float degrees) noexcept
{
    return degrees * kPi / 180.0f;
}
}

OrchestraLayoutComponent::OrchestraLayoutComponent()
{
    setOpaque(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);

    // Exact values supplied in the layout specification.
    // Angles descend from the left baseline (180 degrees) to the right (0).
    sectors = {
        { "Piano", 62, 100, 180,   0, false, false, "PIANO" },
        { "Violin I", 108, 253, 180, 142, false, true, "VIOLIN I" },
        { "Violin II", 110, 192, 142,  90, true, false, "VIOLIN II" },
        { "Violas", 110, 192,  90,  40, false, true, "VIOLAS" },
        { "Celli", 108, 217,  40,   0, true, false, "CELLI" },
        { "Basses", 228, 313,  40,   0, true, false, "BASSES" },
        { "Piccolos", 202, 254, 142, 121, true, false, "PICCOLOS" },
        { "Flutes", 202, 254, 121, 100, false, false, "FLUTES" },
        { "Oboes", 202, 254, 100,  79, false, false, "OBOES" },
        { "Clarinets", 202, 254,  79,  58, false, false, "CLARINETS" },
        { "Bassoons", 202, 254,  58,  40, false, true, "BASSOONS" },
        { "Horns", 262, 318, 180, 143, false, true, "HORNS" },
        { "Harp / Celeste", 262, 318, 143, 111, true, false, "HARP /\nCELESTE" },
        { "Timpani / Untuned Percussion", 262, 318, 111, 69,
          false, false, "TIMPANI /\nUNTUNED PERC" },
        { "Tubular Bells / Marimba, Xylo, Glockenspiel", 262, 318, 69, 40,
          false, true, "TUBULAR BELLS /\nMARIMBA, XYLO,\nGLOCKENSPIEL" },
        { "Trumpets", 326, 388, 40, 30, false, false, "TRUMPETS" },
        { "Tenor Trombones", 326, 388, 30, 20, false, false, "TENOR\nTROMBONES" },
        { "Bass Trombones", 326, 388, 20, 12, false, false, "BASS\nTROMBONES" },
        { "Tuba", 326, 388, 12,  0, false, false, "TUBA" }
    };

    selectedIndex = 3; // Reference image starts with Violas selected.
    sectorPaths.resize(sectors.size());
}

void OrchestraLayoutComponent::resized()
{
    for (std::size_t i = 0; i < sectors.size(); ++i)
        sectorPaths[i] = buildSectorPath(sectors[i]);
}

juce::Path OrchestraLayoutComponent::buildSectorPath(const Sector& sector) const
{
    const float scale = static_cast<float>(getWidth()) / kDesignUnitsAt1200;
    const juce::Point<float> centre(getWidth() * 0.5f, static_cast<float>(getHeight()));
    const float inner = sector.innerRadius * scale;
    const float outer = sector.outerRadius * scale;

    // A gap is made by moving that angular edge 5 px into this sector.
    // At the outer radius this is exactly 5 px; the radial displacement
    // naturally tapers towards the inner radius.
    const float gapAngle = outer > 0.0f
        ? (kGapPixels / outer) * 180.0f / kPi : 0.0f;

    const float startAngle = sector.fromDegrees - (sector.gapAtStart ? gapAngle : 0.0f);
    const float endAngle = sector.toDegrees + (sector.gapAtEnd ? gapAngle : 0.0f);

    juce::Path path;
    constexpr int arcSegments = 48;
    for (int i = 0; i <= arcSegments; ++i)
    {
        const float proportion = static_cast<float>(i) / static_cast<float>(arcSegments);
        const float angle = toRadians(startAngle + (endAngle - startAngle) * proportion);
        const juce::Point<float> point(centre.x + std::cos(angle) * outer,
                                        centre.y - std::sin(angle) * outer);
        if (i == 0)
            path.startNewSubPath(point);
        else
            path.lineTo(point);
    }

    // Radial edge from outer to inner arc.
    const float endRadians = toRadians(endAngle);
    path.lineTo(centre.x + std::cos(endRadians) * inner,
                centre.y - std::sin(endRadians) * inner);

    // Inner arc returns from the sector end angle to its start angle.
    for (int i = arcSegments; i >= 0; --i)
    {
        const float proportion = static_cast<float>(i) / static_cast<float>(arcSegments);
        const float angle = toRadians(startAngle + (endAngle - startAngle) * proportion);
        path.lineTo(centre.x + std::cos(angle) * inner,
                    centre.y - std::sin(angle) * inner);
    }

    path.closeSubPath();
    return path;
}

void OrchestraLayoutComponent::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    // Fill first, then draw all outlines. This keeps adjacent non-gap edges
    // coincident and visually single rather than creating artificial seams.
    for (std::size_t i = 0; i < sectorPaths.size(); ++i)
    {
        const int index = static_cast<int>(i);
        const auto colour = index == selectedIndex ? selectedColour
                          : index == hoveredIndex ? hoverColour : fillColour;
        g.setColour(colour);
        g.fillPath(sectorPaths[i]);
    }

    for (std::size_t i = 0; i < sectorPaths.size(); ++i)
    {
        const int index = static_cast<int>(i);
        const bool selected = index == selectedIndex;
        g.setColour(selected ? selectedOutline : outlineColour);
        g.strokePath(sectorPaths[i],
            juce::PathStrokeType(selected ? 2.5f : 2.0f,
                                 juce::PathStrokeType::curved,
                                 juce::PathStrokeType::rounded));
    }

    const float scale = static_cast<float>(getWidth()) / kDesignUnitsAt1200;
    const juce::Point<float> centre(getWidth() * 0.5f, static_cast<float>(getHeight()));

    g.setColour(labelColour);
    g.setFont(juce::Font(juce::FontOptions(14.0f).withName("Inter")));

    for (const auto& sector : sectors)
    {
        const float midAngle = toRadians((sector.fromDegrees + sector.toDegrees) * 0.5f);
        const float midRadius = (sector.innerRadius + sector.outerRadius) * 0.5f;
        const float radius = midRadius * scale;
        const float x = centre.x + std::cos(midAngle) * radius;
        const float y = centre.y - std::sin(midAngle) * radius;

        const float angularWidth = std::abs(sector.fromDegrees - sector.toDegrees);
        const float availableWidth = juce::jmax(44.0f,
            2.0f * radius * std::sin(toRadians(angularWidth * 0.5f)) - 8.0f);
        const float boxWidth = juce::jmin(availableWidth, 190.0f * scale);
        const int lines = sector.label.retainCharacters("\n").length() + 1;
        const float boxHeight = juce::jmax(20.0f, static_cast<float>(lines) * 17.0f);

        g.drawFittedText(sector.label,
            juce::Rectangle<int>(juce::roundToInt(x - boxWidth * 0.5f),
                                 juce::roundToInt(y - boxHeight * 0.5f),
                                 juce::roundToInt(boxWidth),
                                 juce::roundToInt(boxHeight)),
            juce::Justification::centred, lines, 1.0f);
    }
}

int OrchestraLayoutComponent::findSectorAt(juce::Point<float> point) const
{
    for (int i = static_cast<int>(sectorPaths.size()) - 1; i >= 0; --i)
        if (sectorPaths[static_cast<std::size_t>(i)].contains(point))
            return i;
    return -1;
}

void OrchestraLayoutComponent::updateHover(juce::Point<float> point)
{
    const int next = findSectorAt(point);
    if (next != hoveredIndex)
    {
        hoveredIndex = next;
        repaint();
    }
}

void OrchestraLayoutComponent::mouseMove(const juce::MouseEvent& event)
{
    updateHover(event.position);
}

void OrchestraLayoutComponent::mouseExit(const juce::MouseEvent&)
{
    if (hoveredIndex != -1)
    {
        hoveredIndex = -1;
        repaint();
    }
}

void OrchestraLayoutComponent::mouseDown(const juce::MouseEvent& event)
{
    const int index = findSectorAt(event.position);
    if (index < 0)
        return;

    selectedIndex = index;
    hoveredIndex = index;
    repaint();

    if (onSelectionChanged)
        onSelectionChanged(sectors[static_cast<std::size_t>(index)].name);
}

void OrchestraLayoutComponent::setSelectedSection(const juce::String& name, bool notify)
{
    for (std::size_t i = 0; i < sectors.size(); ++i)
    {
        if (sectors[i].name == name)
        {
            selectedIndex = static_cast<int>(i);
            repaint();
            if (notify && onSelectionChanged)
                onSelectionChanged(sectors[i].name);
            return;
        }
    }
}

juce::String OrchestraLayoutComponent::getSelectedSection() const
{
    return selectedIndex >= 0 && selectedIndex < static_cast<int>(sectors.size())
        ? sectors[static_cast<std::size_t>(selectedIndex)].name : juce::String{};
}
