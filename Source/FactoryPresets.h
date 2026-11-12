#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <map>
#include <vector>

#include "Curve.h"
#include "Modules.h"

namespace sheetz
{
struct Preset
{
    juce::String name;
    juce::String category;
    juce::String author { "SHEETZ!" };
    std::array<std::vector<CurvePoint>, NumModules> curves;
    std::array<bool, NumModules> enabled {};
    std::array<float, NumModules> depth {};
    std::map<juce::String, float> params;
};

std::vector<Preset> createFactoryPresets();
}
