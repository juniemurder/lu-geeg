#include "CurveEditor.h"
#include "CloudsLookAndFeel.h"

#include <cmath>

using namespace juce;

CurveEditor::CurveEditor (SheetzProcessor& p)
    : processor (p)
{
    setOpaque (false);
    setMouseCursor (MouseCursor::CrosshairCursor);
    startTimerHz (30);
}

void CurveEditor::setModule (int newModule)
{
    module = jlimit (0, sheetz::NumModules - 1, newModule);
    dragIndex = -1;
    tensionSegment = -1;
    hoverIndex = -1;
    curveDirty = false;
    repaint();
}

Rectangle<float> CurveEditor::plotArea() const
{
    return getLocalBounds().toFloat().reduced (14.0f, 16.0f);
}

Point<float> CurveEditor::toScreen (float x, float y) const
{
    auto area = plotArea();
    return { area.getX() + x * area.getWidth(),
             area.getY() + (1.0f - y) * area.getHeight() };
}

Point<float> CurveEditor::fromScreen (Point<float> p) const
{
    auto area = plotArea();
    return { jlimit (0.0f, 1.0f, (p.x - area.getX()) / area.getWidth()),
             jlimit (0.0f, 1.0f, 1.0f - (p.y - area.getY()) / area.getHeight()) };
}

int CurveEditor::hitTest (Point<float> p) const
{
    const auto& points = processor.getCurve (module).getPoints();

    for (int i = (int) points.size() - 1; i >= 0; --i)
        if (toScreen (points[(size_t) i].x, points[(size_t) i].y).getDistanceFrom (p) < 11.0f)
            return i;

    return -1;
}

void CurveEditor::timerCallback()
{
    const float phase = processor.getCurrentPhase();

    if (std::abs (phase - lastPhase) > 0.0001f)
    {
        lastPhase = phase;
        repaint();
    }
}

void CurveEditor::paint (Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const auto accentColour = CloudsLookAndFeel::moduleAccent (module);
    auto area = plotArea();

    g.setColour (Colour (0x66FFFFFF));
    g.fillRoundedRectangle (bounds, 16.0f);
    g.setColour (Colour (0x442A6688));
    g.drawRoundedRectangle (bounds, 16.0f, 1.0f);

    g.setFont (Font (FontOptions (9.0f).withStyle ("Bold")));
    g.setColour (CloudsLookAndFeel::ink().withAlpha (0.38f));
    g.drawText (sheetz::moduleAxisTop (module), area.toNearestInt().removeFromTop (11),
                Justification::centredLeft, false);
    g.drawText (sheetz::moduleAxisBottom (module),
                area.toNearestInt().removeFromBottom (11), Justification::centredLeft, false);

    for (int i = 0; i <= 16; ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / 16.0f;
        const bool major = (i % 4) == 0;
        g.setColour (Colour (major ? 0x302A6688 : 0x182A6688));
        g.drawLine (x, area.getY(), x, area.getBottom(), major ? 1.2f : 0.7f);
    }

    for (int i = 0; i <= 8; ++i)
    {
        const float y = area.getY() + area.getHeight() * (float) i / 8.0f;
        const bool major = (i % 4) == 0;
        g.setColour (Colour (major ? 0x302A6688 : 0x182A6688));
        g.drawLine (area.getX(), y, area.getRight(), y, major ? 1.2f : 0.7f);
    }

    const float neutral = sheetz::neutralY (module);
    const float neutralY = toScreen (0.0f, neutral).y;
    const float dashes[] = { 4.0f, 5.0f };
    g.setColour (accentColour.withAlpha (0.30f));
    g.drawDashedLine ({ area.getX(), neutralY, area.getRight(), neutralY }, dashes, 2, 1.0f);

    Path curvePath;

    const int divisions = jmax (64, (int) area.getWidth());

    for (int i = 0; i <= divisions; ++i)
    {
        const float x = (float) i / (float) divisions;
        const auto point = toScreen (x, processor.getCurve (module).evaluate (x));

        if (i == 0)
            curvePath.startNewSubPath (point);
        else
            curvePath.lineTo (point);
    }

    Path fillPath (curvePath);
    fillPath.lineTo (area.getRight(), area.getBottom());
    fillPath.lineTo (area.getX(), area.getBottom());
    fillPath.closeSubPath();

    ColourGradient fill (accentColour.withAlpha (0.30f), area.getCentreX(), area.getY(),
                         accentColour.withAlpha (0.02f), area.getCentreX(), area.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (fillPath);

    g.setColour (accentColour.withAlpha (0.18f));
    g.strokePath (curvePath, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour (accentColour.darker (0.08f));
    g.strokePath (curvePath, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));

    const auto& points = processor.getCurve (module).getPoints();

    for (int i = 0; i < (int) points.size(); ++i)
    {
        const auto& point = points[(size_t) i];
        const auto screenPoint = toScreen (point.x, point.y);
        const bool active = i == dragIndex || i == hoverIndex;

        if (active)
        {
            g.setColour (accentColour.withAlpha (0.22f));
            g.fillEllipse (screenPoint.x - 11.0f, screenPoint.y - 11.0f, 22.0f, 22.0f);
        }

        if (point.hold)
        {
            g.setColour (Colour (0xFFF4B85E));
            g.fillEllipse (screenPoint.x - 5.0f, screenPoint.y - 5.0f, 10.0f, 10.0f);
        }

        g.setColour (Colours::white);
        g.fillEllipse (screenPoint.x - 4.5f, screenPoint.y - 4.5f, 9.0f, 9.0f);
        g.setColour (accentColour.darker (0.15f));
        g.drawEllipse (screenPoint.x - 4.5f, screenPoint.y - 4.5f, 9.0f, 9.0f, 1.8f);
    }

    const float phase = processor.getCurrentPhase();
    const float playheadX = toScreen (phase, 0.0f).x;
    const float playheadY = toScreen (phase, processor.getCurve (module).evaluate (phase)).y;
    g.setColour (Colours::white.withAlpha (0.7f));
    g.drawLine (playheadX, area.getY(), playheadX, area.getBottom(), 1.2f);

    g.setColour (Colour (0xFFFFD27A).withAlpha (0.25f));
    g.fillEllipse (playheadX - 9.0f, playheadY - 9.0f, 18.0f, 18.0f);
    g.setColour (Colour (0xFFFFC85A));
    g.fillEllipse (playheadX - 4.0f, playheadY - 4.0f, 8.0f, 8.0f);

    if (mouseOver)
    {
        const auto position = fromScreen (mousePosition);
        const auto text = String ((int) std::round (position.x * 100.0f)) + "%  /  "
                        + String ((int) std::round (position.y * 100.0f)) + "%";
        auto readout = Rectangle<float> (area.getRight() - 116.0f, area.getY() + 6.0f, 110.0f, 18.0f);

        g.setColour (Colour (0xBFFFFFFF));
        g.fillRoundedRectangle (readout, 6.0f);
        g.setColour (CloudsLookAndFeel::ink().withAlpha (0.7f));
        g.setFont (Font (FontOptions (9.5f).withStyle ("Bold")));
        g.drawText (text, readout.toNearestInt(), Justification::centred, false);
    }
}

void CurveEditor::mouseDown (const MouseEvent& event)
{
    auto& points = processor.getCurve (module).getPoints();
    const auto position = fromScreen (event.position);

    if (event.mods.isPopupMenu() || event.mods.isAltDown())
    {
        tensionSegment = -1;

        for (int i = 0; i + 1 < (int) points.size(); ++i)
        {
            if (position.x >= points[(size_t) i].x && position.x <= points[(size_t) i + 1].x)
            {
                tensionSegment = i;
                tensionStart = points[(size_t) i].tension;
                tensionStartY = event.position.y;
                break;
            }
        }

        return;
    }

    const int hit = hitTest (event.position);

    if (event.mods.isShiftDown() && hit >= 0 && hit < (int) points.size() - 1)
    {
        points[(size_t) hit].hold = ! points[(size_t) hit].hold;
        processor.curveEdited (module);
        processor.curveChangeFinished();
        repaint();
        return;
    }

    if (hit >= 0)
    {
        dragIndex = hit;
        repaint();
        return;
    }

    auto newPoint = position;

    if (snapToGrid && ! event.mods.isShiftDown())
    {
        newPoint.x = std::round (newPoint.x * 16.0f) / 16.0f;
        newPoint.y = std::round (newPoint.y * 8.0f) / 8.0f;
    }

    newPoint.x = jlimit (0.01f, 0.99f, newPoint.x);

    int index = 1;
    while (index < (int) points.size() - 1 && points[(size_t) index].x < newPoint.x)
        ++index;

    points.insert (points.begin() + index, { newPoint.x, newPoint.y, 0.0f, false });
    dragIndex = index;
    curveDirty = true;
    processor.curveEdited (module);
    repaint();
}

void CurveEditor::mouseDrag (const MouseEvent& event)
{
    auto& points = processor.getCurve (module).getPoints();

    if (tensionSegment >= 0 && tensionSegment + 1 < (int) points.size())
    {
        const float delta = (tensionStartY - event.position.y) / 80.0f;
        points[(size_t) tensionSegment].tension = jlimit (-1.0f, 1.0f, tensionStart + delta);
        curveDirty = true;
        processor.curveEdited (module);
        repaint();
        return;
    }

    if (dragIndex < 0 || dragIndex >= (int) points.size())
        return;

    auto position = fromScreen (event.position);

    if (snapToGrid && ! event.mods.isShiftDown())
    {
        position.x = std::round (position.x * 16.0f) / 16.0f;
        position.y = std::round (position.y * 8.0f) / 8.0f;
    }

    auto& point = points[(size_t) dragIndex];
    point.y = jlimit (0.0f, 1.0f, position.y);

    if (dragIndex == 0)
        point.x = 0.0f;
    else if (dragIndex == (int) points.size() - 1)
        point.x = 1.0f;
    else
        point.x = jlimit (points[(size_t) dragIndex - 1].x + 0.003f,
                          points[(size_t) dragIndex + 1].x - 0.003f,
                          position.x);

    curveDirty = true;
    processor.curveEdited (module);
    repaint();
}

void CurveEditor::mouseUp (const MouseEvent&)
{
    dragIndex = -1;
    tensionSegment = -1;

    if (curveDirty)
    {
        curveDirty = false;
        processor.curveChangeFinished();
    }

    repaint();
}

void CurveEditor::mouseDoubleClick (const MouseEvent& event)
{
    auto& points = processor.getCurve (module).getPoints();
    const int hit = hitTest (event.position);

    if (hit > 0 && hit < (int) points.size() - 1)
    {
        points.erase (points.begin() + hit);
        dragIndex = -1;
        processor.curveEdited (module);
        processor.curveChangeFinished();
        repaint();
    }
}

void CurveEditor::mouseMove (const MouseEvent& event)
{
    mousePosition = event.position;
    mouseOver = true;
    hoverIndex = hitTest (event.position);
    repaint();
}

void CurveEditor::mouseExit (const MouseEvent&)
{
    mouseOver = false;
    hoverIndex = -1;
    repaint();
}

void CurveEditor::resetCurve()
{
    processor.getCurve (module).reset (sheetz::neutralY (module));
    processor.curveEdited (module);
    processor.curveChangeFinished();
    repaint();
}

void CurveEditor::flipCurve()
{
    auto& points = processor.getCurve (module).getPoints();

    for (auto& point : points)
        point.y = 1.0f - point.y;

    processor.curveEdited (module);
    processor.curveChangeFinished();
    repaint();
}
