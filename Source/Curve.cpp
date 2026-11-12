#include "Curve.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sheetz
{
namespace
{
float clamp01 (float v) noexcept
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
}

Curve::Curve()
{
    reset (0.5f);
}

void Curve::reset (float y)
{
    y = clamp01 (y);
    points.clear();
    points.push_back ({ 0.0f, y, 0.0f, false });
    points.push_back ({ 1.0f, y, 0.0f, false });
    bake();
}

void Curve::setPoints (std::vector<CurvePoint> newPoints)
{
    points = std::move (newPoints);
    sanitize();
    bake();
}

void Curve::sanitize()
{
    for (auto& p : points)
    {
        p.x = clamp01 (p.x);
        p.y = clamp01 (p.y);
        p.tension = std::max (-1.0f, std::min (1.0f, p.tension));
    }

    std::sort (points.begin(), points.end(),
               [] (const CurvePoint& a, const CurvePoint& b) { return a.x < b.x; });

    std::vector<CurvePoint> cleaned;
    cleaned.reserve (points.size() + 2);

    for (auto p : points)
        if (cleaned.empty() || p.x - cleaned.back().x > 0.0005f)
            cleaned.push_back (p);

    if (cleaned.empty())
        cleaned.push_back ({ 0.0f, 0.5f, 0.0f, false });

    if (cleaned.front().x > 0.0f)
        cleaned.insert (cleaned.begin(), { 0.0f, cleaned.front().y, 0.0f, false });
    else
        cleaned.front().x = 0.0f;

    if (cleaned.size() == 1)
        cleaned.push_back ({ 1.0f, cleaned.front().y, 0.0f, false });
    else if (cleaned.back().x < 1.0f)
        cleaned.push_back ({ 1.0f, cleaned.back().y, 0.0f, false });
    else
        cleaned.back().x = 1.0f;

    points = std::move (cleaned);
}

float Curve::evaluate (float phase) const noexcept
{
    phase = clamp01 (phase);

    if (points.size() < 2)
        return points.empty() ? 0.5f : points.front().y;

    if (phase <= points.front().x)
        return points.front().y;
    if (phase >= points.back().x)
        return points.back().y;

    std::size_t hi = 1;
    while (hi < points.size() - 1 && points[hi].x <= phase)
        ++hi;

    const auto& a = points[hi - 1];
    const auto& b = points[hi];

    if (a.hold)
        return a.y;

    const float span = b.x - a.x;
    float t = span > 1.0e-7f ? (phase - a.x) / span : 1.0f;
    t = clamp01 (t);

    const float smooth = t * t * (3.0f - 2.0f * t);
    const float smoother = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    const float shaped = a.tension < 0.0f
                             ? smooth + a.tension * (smooth - t)
                             : smooth + a.tension * (smoother - smooth);

    return a.y + (b.y - a.y) * clamp01 (shaped);
}

float Curve::read (float phase) const noexcept
{
    phase -= std::floor (phase);

    const float position = phase * (float) tableSize;
    int i = (int) position;
    i &= (tableSize - 1);
    const int j = (i + 1) & (tableSize - 1);
    const float frac = position - std::floor (position);

    const auto& table = tables[activeTable.load (std::memory_order_acquire)];
    return table[(std::size_t) i] + (table[(std::size_t) j] - table[(std::size_t) i]) * frac;
}

void Curve::render (int destinationTable) noexcept
{
    auto& table = tables[(std::size_t) destinationTable];
    for (int i = 0; i < tableSize; ++i)
        table[(std::size_t) i] = evaluate ((float) i / (float) tableSize);
}

void Curve::bake()
{
    const int next = 1 - activeTable.load (std::memory_order_relaxed);
    render (next);
    activeTable.store (next, std::memory_order_release);
}
}
