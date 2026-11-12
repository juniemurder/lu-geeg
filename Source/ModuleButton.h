#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "PluginProcessor.h"

class ModuleButton final : public juce::Component,
                           public juce::SettableTooltipClient,
                           private juce::Timer
{
public:
    ModuleButton (SheetzProcessor&, int moduleIndex);
    ~ModuleButton() override = default;

    void setSelected (bool shouldBeSelected);
    int getModule() const noexcept { return module; }

    std::function<void (int)> onSelect;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    bool powerHit (juce::Point<float>) const;

    SheetzProcessor& processor;
    int module = 0;
    bool selected = false;
    bool hover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleButton)
};
