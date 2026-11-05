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

float radians(float degrees) noexcept { return degrees * kPi / 180.0f; }

juce::Font makeLabelFont(float height, bool bold = false)
{
    juce::FontOptions options;
    auto font = juce::Font(options.withName("Inter").withHeight(height)
                                .withStyle(bold ? "SemiBold" : "Regular"));
    if (!font.getTypefaceName().containsIgnoreCase("Inter"))
        font = juce::Font(options.withName("Arial").withHeight(height)
                              .withStyle(bold ? "Bold" : "Regular"));
    return font;
}
}

OrchestraLayoutComponent::OrchestraLayoutComponent()
{
    setOpaque(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    rebuildSectors();
    juce::Logger::writeToLog("Standard orchestra layout sectors: 19");
}

void OrchestraLayoutComponent::rebuildSectors()
{
    sectors.clear();

    if (currentLayout == Layout::Standard)
    {
        sectors = {
            { "Piano", "Piano", 62, 100, 180, 0, false, false, "PIANO" },
            { "Violins I", "Violins I", 108, 253, 180, 142, false, true, "VIOLINS I" },
            { "Violins II", "Violins II", 110, 192, 142, 90, true, false, "VIOLINS II" },
            { "Violas", "Violas", 110, 192, 90, 40, false, true, "VIOLAS" },
            { "Celli", "Celli", 108, 217, 40, 0, true, false, "CELLI" },
            { "Basses", "Basses", 228, 313, 40, 0, true, false, "BASSES" },
            { "Piccolos", "Piccolos", 202, 254, 142, 121, true, false, "PICCOLOS" },
            { "Flutes", "Flutes", 202, 254, 121, 100, false, false, "FLUTES" },
            { "Oboes", "Oboes", 202, 254, 100, 79, false, false, "OBOES" },
            { "Clarinets", "Clarinets", 202, 254, 79, 58, false, false, "CLARINETS" },
            { "Bassoons", "Bassoons", 202, 254, 58, 40, false, true, "BASSOONS" },
            { "Horns", "Horns", 262, 318, 180, 143, false, true, "HORNS" },
            { "Harp / Celeste", "Harp / Celeste", 262, 318, 143, 111, true, false, "HARP /\nCELESTE" },
            { "Percussion", "Percussion", 262, 318, 111, 69, false, false, "PERCUSSION" },
            { "Tuned Percussion", "Tuned Percussion", 262, 318, 69, 40, false, true, "TUNED\nPERCUSSION" },
            { "Trumpets", "Trumpets", 326, 388, 40, 30, false, false, "TRUMPETS" },
            { "Tenor Trombones", "Tenor Trombones", 326, 388, 30, 20, false, false, "TENOR\nTROMBONES" },
            { "Bass Trombones", "Bass Trombones", 326, 388, 20, 12, false, false, "BASS\nTROMBONES" },
            { "Tuba", "Tuba", 326, 388, 12, 0, false, false, "TUBA" }
        };
    }
    else
    {
        sectors = {
            { "Piano", "Piano", 62, 100, 180, 0, false, false, "PIANO" },
            { "Violins I", "Violins I", 108, 205, 180, 140, false, false, "VIOLINS I" },
            { "Celli", "Celli", 108, 205, 140, 90, false, false, "CELLI" },
            { "Violas", "Violas", 108, 205, 90, 40, false, false, "VIOLAS" },
            { "Violins II", "Violins II", 108, 205, 40, 0, false, false, "VIOLINS II" },
            { "Basses", "Basses", 213, 265, 150, 118, false, false, "BASSES" },
            { "Piccolos / Flutes", "Piccolos", 213, 265, 118, 94, false, false, "PICCOLOS /\nFLUTES" },
            { "Oboes", "Oboes", 213, 265, 94, 70, false, false, "OBOES" },
            { "Clarinets", "Clarinets", 213, 265, 70, 46, false, false, "CLARINETS" },
            { "Bassoons", "Bassoons", 213, 265, 46, 22, false, false, "BASSOONS" },
            { "Harp / Celeste", "Harp / Celeste", 273, 325, 180, 152, false, false, "HARP /\nCELESTE" },
            { "Horns", "Horns", 273, 325, 148, 120, false, false, "HORNS" },
            { "Trumpets", "Trumpets", 273, 325, 120, 98, false, false, "TRUMPETS" },
            { "Tenor Trombones", "Tenor Trombones", 273, 325, 98, 78, false, false, "TENOR\nTROMBONES" },
            { "Bass Trombones", "Bass Trombones", 273, 325, 78, 60, false, false, "BASS\nTROMBONES" },
            { "Tuba", "Tuba", 273, 325, 60, 44, false, false, "TUBA" },
            { "Percussion", "Percussion", 333, 385, 125, 68, false, false, "PERCUSSION" },
            { "Tuned Percussion", "Tuned Percussion", 333, 385, 68, 38, false, false, "TUNED PERCUSSION" }
        };
    }
    else if (currentLayout == Layout::Classical)
    {
        sectors = {
            { "PIANO", "PIANO", 62, 100, 180, 0, false, false, "PIANO" },
            { "VIOLIN I", "VIOLIN I", 108, 205, 180, 140, false, false, "VIOLIN I" },
            { "CELLI", "CELLI", 108, 205, 140, 90, false, false, "CELLI" },
            { "VIOLAS", "VIOLAS", 108, 205, 90, 40, false, false, "VIOLAS" },
            { "VIOLIN II", "VIOLIN II", 108, 205, 40, 0, false, false, "VIOLIN II" },
            { "BASSES", "BASSES", 213, 265, 150, 118, false, false, "BASSES" },
            { "PICCOLOS / FLUTES", "PICCOLOS", 213, 265, 118, 94, false, false, "PICCOLOS /\nFLUTES" },
            { "OBOES", "OBOES", 213, 265, 94, 70, false, false, "OBOES" },
            { "CLARINETS", "CLARINETS", 213, 265, 70, 46, false, false, "CLARINETS" },
            { "BASSOONS", "BASSOONS", 213, 265, 46, 22, false, false, "BASSOONS" },
            { "HARP / CELESTE", "HARP / CELESTE", 273, 325, 180, 152, false, false, "HARP /\nCELESTE" },
            { "HORNS", "HORNS", 273, 325, 148, 120, false, false, "HORNS" },
            { "TRUMPETS", "TRUMPETS", 273, 325, 120, 98, false, false, "TRUMPETS" },
            { "TENOR TROMBONES", "TENOR TROMBONES", 273, 325, 98, 78, false, false, "TENOR\nTROMBONES" },
            { "BASS TROMBONES", "BASS TROMBONES", 273, 325, 78, 60, false, false, "BASS\nTROMBONES" },
            { "TUBA", "TUBA", 273, 325, 60, 44, false, false, "TUBA" },
            { "PERCUSSION", "PERCUSSION", 333, 385, 125, 68, false, false, "PERCUSSION" },
            { "TUNED PERCUSSION", "TUNED PERCUSSION", 333, 385, 68, 38, false, false, "TUNED PERCUSSION" }
        };
    }
    else if (currentLayout == Layout::ChamberModern)
    {
        sectors = {
            { "PIANO", "PIANO", 62, 100, 180, 0, false, false, "PIANO" },
            { "VIOLIN I", "VIOLIN I", 108, 165, 180, 135, false, false, "VIOLIN I" },
            { "VIOLIN II", "VIOLIN II", 108, 165, 135, 90, false, false, "VIOLIN II" },
            { "VIOLAS", "VIOLAS", 108, 165, 90, 45, false, false, "VIOLAS" },
            { "CELLI", "CELLI", 108, 165, 45, 0, false, false, "CELLI" },
            { "FLUTES", "FLUTES", 173, 225, 180, 146, false, false, "FLUTES" },
            { "OBOES", "OBOES", 173, 225, 146, 112, false, false, "OBOES" },
            { "CLARINETS", "CLARINETS", 173, 225, 112, 78, false, false, "CLARINETS" },
            { "BASSOONS", "BASSOONS", 173, 225, 78, 45, false, false, "BASSOONS" },
            { "BASSES", "BASSES", 173, 225, 45, 0, false, false, "BASSES" },
            { "HORNS", "HORNS", 233, 285, 150, 108, false, false, "HORNS" },
            { "TRUMPETS", "TRUMPETS", 233, 285, 108, 66, false, false, "TRUMPETS" },
            { "TIMPANI", "TIMPANI", 293, 345, 120, 60, false, false, "TIMPANI" }
        };
    }
    else
    {
        sectors = {
            { "HARPSICHORD / FORTEPIANO", "HARPSICHORD / FORTEPIANO", 62, 100, 180, 0, false, false, "HARPSICHORD /\nFORTEPIANO" },
            { "VIOLIN I", "VIOLIN I", 108, 165, 180, 140, false, false, "VIOLIN I" },
            { "CELLI", "CELLI", 108, 165, 140, 90, false, false, "CELLI" },
            { "VIOLAS", "VIOLAS", 108, 165, 90, 40, false, false, "VIOLAS" },
            { "VIOLIN II", "VIOLIN II", 108, 165, 40, 0, false, false, "VIOLIN II" },
            { "BASSES", "BASSES", 173, 225, 150, 122, false, false, "BASSES" },
            { "FLUTES", "FLUTES", 173, 225, 122, 98, false, false, "FLUTES" },
            { "OBOES", "OBOES", 173, 225, 98, 74, false, false, "OBOES" },
            { "CLARINETS", "CLARINETS", 173, 225, 74, 50, false, false, "CLARINETS" },
            { "BASSOONS", "BASSOONS", 173, 225, 50, 26, false, false, "BASSOONS" },
            { "HORNS", "HORNS", 233, 285, 126, 96, false, false, "HORNS" },
            { "TRUMPETS", "TRUMPETS", 233, 285, 96, 66, false, false, "TRUMPETS" },
            { "TIMPANI", "TIMPANI", 293, 345, 110, 70, false, false, "TIMPANI" }
        };
    }

    selectedIndex = juce::jlimit(0, static_cast<int>(sectors.size()) - 1, selectedIndex);
    sectorPaths.resize(sectors.size());
    resized();
}

float OrchestraLayoutComponent::getLayoutScale() const noexcept
{
    if (currentLayout == Layout::Standard)
        return juce::jmin(static_cast<float>(getWidth()) / 750.0f,
                          static_cast<float>(getHeight()) / 388.0f);
    if (currentLayout == Layout::Classical)
        return 1.35f * juce::jmin(static_cast<float>(getWidth()) / 1200.0f,
                                  static_cast<float>(getHeight()) / 519.75f);
    return 1.5f * juce::jmin(static_cast<float>(getWidth()) / 1200.0f,
                             static_cast<float>(getHeight()) / 660.0f);
}

void OrchestraLayoutComponent::resized()
{
    for (std::size_t i = 0; i < sectors.size(); ++i)
        sectorPaths[i] = buildSectorPath(sectors[i]);
}

juce::Path OrchestraLayoutComponent::buildSectorPath(const Sector& sector) const
{
    const float scale = getLayoutScale();
    const float inner = sector.innerRadius * scale;
    const float outer = sector.outerRadius * scale;
    const juce::Point<float> centre(getWidth() * 0.5f, static_cast<float>(getHeight()));

    auto shiftedAngle = [](float degrees, float radius, bool isStart, bool hasGap)
    {
        if (!hasGap || radius <= 0.0f)
            return radians(degrees);
        const float delta = std::asin(juce::jlimit(0.0f, 0.999f, 5.0f / radius));
        return radians(degrees) + (isStart ? -delta : delta);
    };

    const float outerStart = shiftedAngle(sector.fromDegrees, outer, true, sector.gapAtStart);
    const float outerEnd   = shiftedAngle(sector.toDegrees, outer, false, sector.gapAtEnd);
    const float innerStart = shiftedAngle(sector.fromDegrees, inner, true, sector.gapAtStart);
    const float innerEnd   = shiftedAngle(sector.toDegrees, inner, false, sector.gapAtEnd);

    juce::Path path;
    constexpr int arcSegments = 72;
    for (int i = 0; i <= arcSegments; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(arcSegments);
        const float angle = outerStart + (outerEnd - outerStart) * t;
        const juce::Point<float> point(centre.x + std::cos(angle) * outer,
                                       centre.y - std::sin(angle) * outer);
        if (i == 0) path.startNewSubPath(point); else path.lineTo(point);
    }

    path.lineTo(centre.x + std::cos(innerEnd) * inner,
                centre.y - std::sin(innerEnd) * inner);

    for (int i = 0; i <= arcSegments; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(arcSegments);
        const float angle = innerEnd + (innerStart - innerEnd) * t;
        path.lineTo(centre.x + std::cos(angle) * inner,
                    centre.y - std::sin(angle) * inner);
    }

    path.closeSubPath();
    return path;
}

void OrchestraLayoutComponent::paint(juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    for (const auto& path : sectorPaths)
    {
        g.setColour(fillColour);
        g.fillPath(path);
    }
    const juce::PathStrokeType outline(2.0f, juce::PathStrokeType::curved,
                                       juce::PathStrokeType::rounded);
    for (const auto& path : sectorPaths)
    {
        g.setColour(outlineColour);
        g.strokePath(path, outline);
    }

    auto drawState = [this, &g](int index, juce::Colour colour, float thickness)
    {
        if (index < 0 || index >= static_cast<int>(sectorPaths.size()))
            return;
        g.setColour(colour);
        g.fillPath(sectorPaths[static_cast<std::size_t>(index)]);
        g.setColour(selectedOutline);
        g.strokePath(sectorPaths[static_cast<std::size_t>(index)],
                     juce::PathStrokeType(thickness, juce::PathStrokeType::curved,
                                          juce::PathStrokeType::rounded));
    };
    if (hoveredIndex >= 0 && hoveredIndex != selectedIndex)
        drawState(hoveredIndex, hoverColour, 2.0f);
    if (selectedIndex >= 0)
        drawState(selectedIndex, selectedColour, 2.5f);

    const float scale = getLayoutScale();
    const juce::Point<float> centre(getWidth() * 0.5f, static_cast<float>(getHeight()));
    for (std::size_t i = 0; i < sectors.size(); ++i)
    {
        const auto& sector = sectors[i];
        const float midAngle = radians((sector.fromDegrees + sector.toDegrees) * 0.5f);
        const float radius = (sector.innerRadius + sector.outerRadius) * 0.5f * scale;
        const juce::Point<float> anchor(centre.x + std::cos(midAngle) * radius,
                                        centre.y - std::sin(midAngle) * radius);
        const float angularWidth = std::abs(sector.fromDegrees - sector.toDegrees);
        const float localWidth = juce::jmax(30.0f,
            2.0f * radius * std::sin(radians(angularWidth * 0.5f)) - 8.0f);
        const int lineCount = sector.label.contains("\n") ? 2 : 1;
        float fontSize = 14.0f;
        auto font = makeLabelFont(fontSize, static_cast<int>(i) == selectedIndex);
        auto widthOf = [&font](const juce::String& line)
        {
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText(font, line, 0.0f, 0.0f);
            return juce::jmax(static_cast<float>(font.getStringWidth(line)),
                              glyphs.getBoundingBox(0, -1, true).getWidth());
        };
        juce::String lines[2] { sector.label.upToFirstOccurrenceOf("\n", false, false),
                                sector.label.fromFirstOccurrenceOf("\n", false, false) };
        auto widest = [&]()
        {
            float width = widthOf(lines[0]);
            if (lineCount == 2) width = juce::jmax(width, widthOf(lines[1]));
            return width;
        };
        while (widest() > localWidth && fontSize > 9.0f)
        {
            fontSize = juce::jmax(9.0f, fontSize - 0.5f);
            font = makeLabelFont(fontSize, static_cast<int>(i) == selectedIndex);
        }
        const float lineHeight = font.getHeight();
        const float boxHeight = lineHeight * static_cast<float>(lineCount);
        juce::Rectangle<float> box(anchor.x - localWidth * 0.5f,
                                   anchor.y - boxHeight * 0.5f,
                                   localWidth, boxHeight);
        const auto& path = sectorPaths[i];
        auto cornersInside = [&]()
        {
            return path.contains(box.getTopLeft()) && path.contains(box.getTopRight())
                && path.contains(box.getBottomLeft()) && path.contains(box.getBottomRight());
        };
        while (!cornersInside() && fontSize > 9.0f)
        {
            fontSize = juce::jmax(9.0f, fontSize - 0.5f);
            font = makeLabelFont(fontSize, static_cast<int>(i) == selectedIndex);
            const float h = font.getHeight() * static_cast<float>(lineCount);
            box.setY(anchor.y - h * 0.5f);
            box.setHeight(h);
        }
        if (!cornersInside())
            juce::Logger::writeToLog("Orchestra label bounds outside sector: " + sector.name);

        g.setColour(labelColour);
        g.setFont(font);
        g.drawText(lines[0], box.getX(), box.getY(), box.getWidth(), lineHeight,
                   juce::Justification::centred, false);
        if (lineCount == 2)
            g.drawText(lines[1], box.getX(), box.getY() + lineHeight, box.getWidth(),
                       lineHeight, juce::Justification::centred, false);
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
    if (next != hoveredIndex) { hoveredIndex = next; repaint(); }
}

void OrchestraLayoutComponent::mouseMove(const juce::MouseEvent& event) { updateHover(event.position); }
void OrchestraLayoutComponent::mouseExit(const juce::MouseEvent&) { if (hoveredIndex != -1) { hoveredIndex = -1; repaint(); } }

void OrchestraLayoutComponent::mouseDown(const juce::MouseEvent& event)
{
    const int index = findSectorAt(event.position);
    if (index < 0) return;
    selectedIndex = index;
    hoveredIndex = index;
    repaint();
    if (onSelectionChanged) onSelectionChanged(getSelectedParameterName());
}

void OrchestraLayoutComponent::setLayout(Layout layout, bool notify)
{
    if (layout == currentLayout) return;
    const auto previousSelection = getSelectedParameterName();
    currentLayout = layout;
    hoveredIndex = -1;
    rebuildSectors();
    int match = -1;
    for (std::size_t i = 0; i < sectors.size(); ++i)
        if (sectors[i].parameterName == previousSelection
            || (currentLayout == Layout::Classical && previousSelection == "Flutes"
                && sectors[i].name == "Piccolos / Flutes"))
        { match = static_cast<int>(i); break; }
    selectedIndex = match >= 0 ? match : juce::jmin(1, static_cast<int>(sectors.size()) - 1);
    juce::Logger::writeToLog("Orchestra layout sector count: " + juce::String(sectors.size()));
    jassert((currentLayout == Layout::ChamberModern || currentLayout == Layout::ChamberClassical)
                ? sectors.size() == 13 : true);
    resized();
    repaint();
    if (notify && onLayoutChanged) onLayoutChanged(currentLayout);
    if (notify && onSelectionChanged) onSelectionChanged(getSelectedParameterName());
}

void OrchestraLayoutComponent::setSelectedSection(const juce::String& name, bool notify)
{
    for (std::size_t i = 0; i < sectors.size(); ++i)
    {
        if (sectors[i].parameterName == name || sectors[i].name == name
            || (currentLayout == Layout::Classical && name == "Flutes"
                && sectors[i].name == "Piccolos / Flutes"))
        {
            selectedIndex = static_cast<int>(i);
            repaint();
            if (notify && onSelectionChanged) onSelectionChanged(getSelectedParameterName());
            return;
        }
    }
}

juce::String OrchestraLayoutComponent::getSelectedSection() const
{
    return selectedIndex >= 0 && selectedIndex < static_cast<int>(sectors.size())
        ? sectors[static_cast<std::size_t>(selectedIndex)].name : juce::String{};
}

juce::String OrchestraLayoutComponent::getSelectedParameterName() const
{
    return selectedIndex >= 0 && selectedIndex < static_cast<int>(sectors.size())
        ? sectors[static_cast<std::size_t>(selectedIndex)].parameterName : juce::String{};
}

juce::String OrchestraLayoutComponent::getTitle() const
{
    switch (currentLayout)
    {
        case Layout::Standard: return "FULL ORCHESTRA - MODERN";
        case Layout::Classical: return "FULL ORCHESTRA - CLASSICAL (ANTIPHONAL)";
        case Layout::ChamberModern: return "CHAMBER ORCHESTRA - MODERN";
        case Layout::ChamberClassical: return "CHAMBER ORCHESTRA - CLASSICAL (ANTIPHONAL)";
    }
    return {};
}
