#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class CurveEditor final : public juce::Component,
                          private juce::Timer
{
public:
    explicit CurveEditor (SheetzProcessor&);
    ~CurveEditor() override = default;

    void setModule (int newModule);
    int getModule() const noexcept { return module; }

    void setSnap (bool shouldSnap) { snapToGrid = shouldSnap; }
    bool getSnap() const noexcept { return snapToGrid; }

    void resetCurve();
    void flipCurve();

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> plotArea() const;
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> fromScreen (juce::Point<float> point) const;
    int hitTest (juce::Point<float> point) const;
    void timerCallback() override;

    SheetzProcessor& processor;
    int module = 0;
    bool snapToGrid = true;
    bool curveDirty = false;
    int dragIndex = -1;
    int tensionSegment = -1;
    float tensionStart = 0.0f;
    float tensionStartY = 0.0f;
    int hoverIndex = -1;
    juce::Point<float> mousePosition;
    bool mouseOver = false;
    float lastPhase = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CurveEditor)
};
