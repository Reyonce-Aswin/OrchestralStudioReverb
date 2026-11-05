#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class OrchestraLayoutComponent final : public juce::Component
{
public:
    OrchestraLayoutComponent();

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setSelectedSection(const juce::String& name, bool notify = false);
    [[nodiscard]] juce::String getSelectedSection() const;

    std::function<void(const juce::String&)> onSelectionChanged;

private:
    struct Sector
    {
        juce::String name;
        float innerRadius = 0.0f;
        float outerRadius = 0.0f;
        float fromDegrees = 0.0f;
        float toDegrees = 0.0f;
        bool gapAtStart = false;
        bool gapAtEnd = false;
        juce::String label;
    };

    juce::Path buildSectorPath(const Sector& sector) const;
    int findSectorAt(juce::Point<float> point) const;
    void updateHover(juce::Point<float> point);

    std::vector<Sector> sectors;
    std::vector<juce::Path> sectorPaths;
    int selectedIndex = -1;
    int hoveredIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestraLayoutComponent)
};
