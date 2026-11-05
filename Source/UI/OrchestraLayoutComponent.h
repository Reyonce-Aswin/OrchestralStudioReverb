#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

class OrchestraLayoutComponent final : public juce::Component
{
public:
    enum class Layout { Standard, Classical, ChamberModern, ChamberClassical };

    OrchestraLayoutComponent();
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;

    void setLayout(Layout, bool notify = false);
    Layout getLayout() const noexcept { return currentLayout; }
    juce::String getTitle() const;
    void setSelectedSection(const juce::String&, bool notify = false);
    juce::String getSelectedSection() const;
    juce::String getSelectedParameterName() const;

    std::function<void(const juce::String&)> onSelectionChanged;
    std::function<void(Layout)> onLayoutChanged;

private:
    struct Sector
    {
        juce::String name;
        juce::String parameterName;
        float innerRadius = 0.0f, outerRadius = 0.0f;
        float fromDegrees = 0.0f, toDegrees = 0.0f;
        bool gapAtStart = false, gapAtEnd = false;
        juce::String label;
    };

    void rebuildSectors();
    juce::Path buildSectorPath(const Sector&) const;
    int findSectorAt(juce::Point<float>) const;
    void updateHover(juce::Point<float>);
    float getLayoutScale() const noexcept;

    std::vector<Sector> sectors;
    std::vector<juce::Path> sectorPaths;
    Layout currentLayout = Layout::Standard;
    int selectedIndex = 1;
    int hoveredIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrchestraLayoutComponent)
};
