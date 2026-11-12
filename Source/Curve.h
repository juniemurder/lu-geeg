#pragma once

#include <array>
#include <atomic>
#include <vector>

namespace sheetz
{
struct CurvePoint
{
    float x = 0.0f;
    float y = 0.5f;
    float tension = 0.0f; // -1 = linear, 0 = smooth, +1 = extra smooth
    bool hold = false;    // hold this value until the next point (gate/step)
};

/** A tempo-synced breakpoint curve with a lock-free double-buffered bake table. */
class Curve
{
public:
    static constexpr int tableSize = 2048;

    Curve();

    void reset (float y = 0.5f);
    void setPoints (std::vector<CurvePoint> newPoints);
    void sanitize();
    void bake();

    const std::vector<CurvePoint>& getPoints() const noexcept { return points; }
    std::vector<CurvePoint>& getPoints() noexcept { return points; }

    float evaluate (float phase) const noexcept;
    float read (float phase) const noexcept;

private:
    void render (int destinationTable) noexcept;

    std::vector<CurvePoint> points;
    std::array<float, tableSize> tables[2] {};
    std::atomic<int> activeTable { 0 };
};
}
