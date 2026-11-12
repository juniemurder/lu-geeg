#include "ModuleButton.h"
#include "CloudsLookAndFeel.h"

using namespace juce;

ModuleButton::ModuleButton (SheetzProcessor& p, int moduleIndex)
    : processor (p), module (jlimit (0, sheetz::NumModules - 1, moduleIndex))
{
    setOpaque (false);
    setMouseCursor (MouseCursor::PointingHandCursor);
    setTooltip ("Click to edit. Click the dot to bypass.");
    startTimerHz (12);
}

void ModuleButton::setSelected (bool shouldBeSelected)
{
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }
}

void ModuleButton::timerCallback()
{
    repaint();
}

bool ModuleButton::powerHit (Point<float> point) const
{
    const auto dot = Rectangle<float> ((float) getWidth() - 24.0f, 8.0f, 16.0f, 16.0f);
    return dot.expanded (6.0f).contains (point);
}

void ModuleButton::paint (Graphics& g)
{
    const auto accent = CloudsLookAndFeel::moduleAccent (module);
    const auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    const auto id = String (sheetz::moduleId (module));

    const bool enabled = processor.apvts.getRawParameterValue (id + "_on")->load() > 0.5f;
    const float depth = processor.apvts.getRawParameterValue (id + "_depth")->load() * 0.01f;

    g.setColour (Colour (selected ? 0xF2FFFFFF : (hover ? 0x99FFFFFF : 0x5EFFFFFF)));
    g.fillRoundedRectangle (bounds, 14.0f);

    if (selected)
    {
        g.setColour (Colour (0x2214384F));
        g.drawRoundedRectangle (bounds.translated (0.0f, 1.5f), 14.0f, 1.0f);
        g.setColour (accent.withAlpha (0.85f));
        g.drawRoundedRectangle (bounds, 14.0f, 1.5f);
    }
    else
    {
        g.setColour (Colour (0x33FFFFFF));
        g.drawRoundedRectangle (bounds, 14.0f, 1.0f);
    }

    g.setColour (accent.withAlpha (selected ? 0.95f : 0.65f));
    g.fillRoundedRectangle (bounds.getX() + 2.0f, bounds.getY() + 13.0f, 3.5f, bounds.getHeight() - 26.0f, 2.0f);

    const float textAlpha = enabled ? 1.0f : 0.42f;

    g.setFont (Font (FontOptions (12.5f).withStyle ("Bold")));
    g.setColour (CloudsLookAndFeel::ink().withAlpha (textAlpha));
    g.drawText (sheetz::moduleName (module),
                getLocalBounds().reduced (12, 0).removeFromTop (27).translated (0, 2),
                Justification::centredLeft, false);

    const float dotX = (float) getWidth() - 20.0f;
    const float dotY = 14.0f;

    if (enabled)
    {
        g.setColour (accent.withAlpha (0.25f));
        g.fillEllipse (dotX - 7.0f, dotY - 7.0f, 14.0f, 14.0f);
        g.setColour (accent);
        g.fillEllipse (dotX - 4.5f, dotY - 4.5f, 9.0f, 9.0f);
    }
    else
    {
        g.setColour (CloudsLookAndFeel::ink().withAlpha (0.35f));
        g.drawEllipse (dotX - 4.5f, dotY - 4.5f, 9.0f, 9.0f, 1.4f);
    }

    auto curveArea = getLocalBounds().toFloat().reduced (12.0f, 0.0f);
    curveArea = curveArea.withTrimmedTop (31.0f).withTrimmedBottom (11.0f);

    g.setColour (CloudsLookAndFeel::ink().withAlpha (0.12f));
    g.drawLine (curveArea.getX(), curveArea.getBottom(), curveArea.getRight(), curveArea.getBottom(), 1.0f);

    Path line;

    for (int i = 0; i <= 36; ++i)
    {
        const float x = (float) i / 36.0f;
        const float value = processor.getCurve (module).evaluate (x);
        const Point<float> point (curveArea.getX() + x * curveArea.getWidth(),
                                  curveArea.getBottom() - value * curveArea.getHeight());

        if (i == 0)
            line.startNewSubPath (point);
        else
            line.lineTo (point);
    }

    g.setColour (accent.withAlpha (enabled ? 0.92f : 0.30f));
    g.strokePath (line, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));

    if (depth > 0.001f)
    {
        g.setColour (accent.withAlpha (0.20f));
        g.fillRoundedRectangle (curveArea.getX(), curveArea.getBottom() + 5.0f,
                                curveArea.getWidth(), 2.0f, 1.0f);
        g.setColour (accent.withAlpha (enabled ? 0.75f : 0.3f));
        g.fillRoundedRectangle (curveArea.getX(), curveArea.getBottom() + 5.0f,
                                curveArea.getWidth() * depth, 2.0f, 1.0f);
    }
}

void ModuleButton::mouseDown (const MouseEvent& event)
{
    if (powerHit (event.position))
    {
        const auto id = String (sheetz::moduleId (module)) + "_on";

        if (auto* parameter = processor.apvts.getParameter (id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->getValue() > 0.5f ? 0.0f : 1.0f);
            parameter->endChangeGesture();
        }

        repaint();
        return;
    }

    if (onSelect != nullptr)
        onSelect (module);
}

void ModuleButton::mouseEnter (const MouseEvent&)
{
    hover = true;
    repaint();
}

void ModuleButton::mouseExit (const MouseEvent&)
{
    hover = false;
    repaint();
}
