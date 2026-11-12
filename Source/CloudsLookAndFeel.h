#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Modules.h"

class CloudsLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    CloudsLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;

    static juce::Colour skyTop();
    static juce::Colour skyBottom();
    static juce::Colour ink();
    static juce::Colour accent();
    static juce::Colour cloudWhite();
    static juce::Colour moduleAccent (int module);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CloudsLookAndFeel)
};
