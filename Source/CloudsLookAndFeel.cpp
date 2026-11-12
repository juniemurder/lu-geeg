#include "CloudsLookAndFeel.h"

#include <cmath>

using namespace juce;

CloudsLookAndFeel::CloudsLookAndFeel()
{
    setColour (Label::textColourId, ink());
    setColour (ComboBox::backgroundColourId, Colour (0xE6FFFFFF));
    setColour (ComboBox::textColourId, ink());
    setColour (ComboBox::outlineColourId, Colour (0x552A6688));
    setColour (ComboBox::arrowColourId, accent().darker (0.15f));
    setColour (ComboBox::focusedOutlineColourId, accent());
    setColour (PopupMenu::backgroundColourId, Colour (0xFFF4FCFF));
    setColour (PopupMenu::textColourId, ink());
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xFFC9EEFF));
    setColour (PopupMenu::highlightedTextColourId, Colour (0xFF173F57));
    setColour (PopupMenu::headerTextColourId, accent().darker (0.2f));
    setColour (TextButton::buttonColourId, Colour (0xCCFFFFFF));
    setColour (TextButton::buttonOnColourId, accent());
    setColour (TextButton::textColourOffId, ink());
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (Slider::rotarySliderFillColourId, accent());
    setColour (Slider::textBoxTextColourId, ink());
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentWhite);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentWhite);
    setColour (TooltipWindow::backgroundColourId, Colour (0xF2FFFFFF));
    setColour (TooltipWindow::textColourId, ink());
    setColour (TooltipWindow::outlineColourId, Colour (0x332A6688));
}

Colour CloudsLookAndFeel::skyTop()       { return Colour (0xFFBCE8FF); }
Colour CloudsLookAndFeel::skyBottom()    { return Colour (0xFFF0FBFF); }
Colour CloudsLookAndFeel::ink()          { return Colour (0xFF24536F); }
Colour CloudsLookAndFeel::accent()       { return Colour (0xFF3E9FD3); }
Colour CloudsLookAndFeel::cloudWhite()   { return Colour (0xFFF9FDFF); }

Colour CloudsLookAndFeel::moduleAccent (int module)
{
    switch (module)
    {
        case sheetz::Volume: return Colour (0xFF4FB3E8);
        case sheetz::Filter: return Colour (0xFF9B8CFF);
        case sheetz::Pan:    return Colour (0xFF5FD6B4);
        case sheetz::Width:  return Colour (0xFFFFB38A);
        case sheetz::Drive:  return Colour (0xFFFF8E8E);
        case sheetz::Crush:  return Colour (0xFFEFC75E);
        default:             return accent();
    }
}

void CloudsLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, Slider& slider)
{
    auto area = Rectangle<int> (x, y, width, height).toFloat().reduced (5.0f);
    const float diameter = jmin (area.getWidth(), area.getHeight());
    const float cx = area.getCentreX();
    const float cy = area.getCentreY();
    const float radius = diameter * 0.5f;
    const float trackRadius = jmax (4.0f, radius - 4.5f);
    const float bodyRadius = jmax (6.0f, radius - 11.0f);
    const float angle = rotaryStartAngle + jlimit (0.0f, 1.0f, sliderPosProportional)
                                             * (rotaryEndAngle - rotaryStartAngle);

    const auto fill = slider.findColour (Slider::rotarySliderFillColourId);
    const float stroke = jmax (3.0f, diameter * 0.055f);

    Path track;
    track.addCentredArc (cx, cy, trackRadius, trackRadius, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Colour (0x3024536F));
    g.strokePath (track, PathStrokeType (stroke, PathStrokeType::curved, PathStrokeType::rounded));

    if (sliderPosProportional > 0.001f)
    {
        Path value;
        value.addCentredArc (cx, cy, trackRadius, trackRadius, 0.0f,
                             rotaryStartAngle, angle, true);
        g.setColour (fill);
        g.strokePath (value, PathStrokeType (stroke, PathStrokeType::curved, PathStrokeType::rounded));
    }

    g.setColour (Colour (0x2414384F));
    g.fillEllipse (cx - bodyRadius + 0.5f, cy - bodyRadius + 3.0f, bodyRadius * 2.0f, bodyRadius * 2.0f);

    ColourGradient body (Colours::white, cx - bodyRadius, cy - bodyRadius,
                         Colour (0xFFE4F6FF), cx + bodyRadius, cy + bodyRadius, false);
    g.setGradientFill (body);
    g.fillEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    g.setColour (Colour (0x88FFFFFF));
    g.drawEllipse (cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f, 1.0f);

    const Point<float> tip (cx + std::sin (angle) * (bodyRadius - 4.0f),
                            cy - std::cos (angle) * (bodyRadius - 4.0f));
    g.setColour (fill.darker (0.35f));
    g.drawLine (cx, cy, tip.x, tip.y, jmax (1.8f, diameter * 0.025f));
    g.fillEllipse (cx - 2.2f, cy - 2.2f, 4.4f, 4.4f);
}

void CloudsLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool,
                                      int buttonX, int, int buttonW, int, ComboBox& box)
{
    auto bounds = Rectangle<float> (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f);
    const bool hover = box.isMouseOver (true);

    g.setColour (Colour (hover ? 0xF2FFFFFF : 0xD9FFFFFF));
    g.fillRoundedRectangle (bounds, 9.0f);
    g.setColour (Colour (0x442A6688));
    g.drawRoundedRectangle (bounds, 9.0f, 1.0f);

    const float cx = (float) buttonX + (float) buttonW * 0.5f;
    const float cy = (float) height * 0.5f;

    Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.5f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (accent().darker (0.2f));
    g.strokePath (chevron, PathStrokeType (1.7f, PathStrokeType::curved, PathStrokeType::rounded));
}

void CloudsLookAndFeel::positionComboBoxText (ComboBox& box, Label& label)
{
    label.setBounds (12, 1, box.getWidth() - 30, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (Justification::centredLeft);
}

void CloudsLookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto colour = backgroundColour;

    if (button.getToggleState())
        colour = button.findColour (TextButton::buttonOnColourId);

    if (shouldDrawButtonAsDown)
        colour = colour.darker (0.07f);
    else if (shouldDrawButtonAsHighlighted)
        colour = colour.brighter (0.04f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 9.0f);

    g.setColour (button.getToggleState() ? Colour (0x55FFFFFF) : Colour (0x552A6688));
    g.drawRoundedRectangle (bounds, 9.0f, 1.0f);
}

Font CloudsLookAndFeel::getTextButtonFont (TextButton&, int)
{
    return Font (FontOptions (12.5f).withStyle ("Bold"));
}

Font CloudsLookAndFeel::getComboBoxFont (ComboBox&)
{
    return Font (FontOptions (13.0f));
}

Font CloudsLookAndFeel::getLabelFont (Label&)
{
    return Font (FontOptions (13.0f));
}
