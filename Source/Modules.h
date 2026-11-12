#pragma once

namespace sheetz
{
enum Module
{
    Volume = 0,
    Filter,
    Pan,
    Width,
    Drive,
    Crush,
    NumModules
};

inline const char* moduleId (int m) noexcept
{
    static const char* const ids[] = { "volume", "filter", "pan", "width", "drive", "crush" };
    return ids[(m >= 0 && m < NumModules) ? m : 0];
}

inline const char* moduleName (int m) noexcept
{
    static const char* const names[] = { "Volume", "Filter", "Pan", "Width", "Drive", "Crush" };
    return names[(m >= 0 && m < NumModules) ? m : 0];
}

// The resting curve value for each module.  Volume is unity, the bipolar
// modules rest at the centre, and the two texture modules rest at zero.
inline float neutralY (int m) noexcept
{
    switch (m)
    {
        case Volume: return 1.0f;
        case Filter: return 0.5f;
        case Pan:    return 0.5f;
        case Width:  return 0.5f;
        default:     return 0.0f;
    }
}

inline const char* moduleAxisTop (int m) noexcept
{
    switch (m)
    {
        case Volume: return "LOUD";
        case Filter: return "OPEN";
        case Pan:    return "RIGHT";
        case Width:  return "WIDE";
        case Drive:  return "HOT";
        case Crush:  return "CRUSHED";
        default:     return "";
    }
}

inline const char* moduleAxisBottom (int m) noexcept
{
    switch (m)
    {
        case Volume: return "SILENT";
        case Filter: return "DARK";
        case Pan:    return "LEFT";
        case Width:  return "MONO";
        case Drive:  return "CLEAN";
        case Crush:  return "CLEAN";
        default:     return "";
    }
}
}
