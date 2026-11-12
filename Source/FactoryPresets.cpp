#include "FactoryPresets.h"

#include <initializer_list>

namespace sheetz
{
namespace
{
Preset base (const juce::String& name, const juce::String& category)
{
    Preset p;
    p.name = name;
    p.category = category;
    p.params["mix"] = 100.0f;
    p.params["smooth"] = 4.0f;

    for (int m = 0; m < NumModules; ++m)
        p.curves[(size_t) m] = { { 0.0f, neutralY (m), 0.0f, false },
                                 { 1.0f, neutralY (m), 0.0f, false } };

    return p;
}

void shape (Preset& p, int m, float depth, std::initializer_list<CurvePoint> points)
{
    p.enabled[(size_t) m] = true;
    p.depth[(size_t) m] = depth;
    p.curves[(size_t) m].assign (points.begin(), points.end());
}

void set (Preset& p, const char* id, float value)
{
    p.params[id] = value;
}

std::vector<Preset> build()
{
    std::vector<Preset> presets;

    {
        auto p = base ("Cloud Nine", "Tremolo");
        set (p, "rate", 5.0f);
        set (p, "smooth", 6.0f);
        shape (p, Volume, 55.0f, { { 0.0f, 0.20f, 0.0f }, { 0.25f, 1.0f, 0.0f },
                                   { 0.50f, 0.20f, 0.0f }, { 0.75f, 1.0f, 0.0f },
                                   { 1.0f, 0.20f, 0.0f } });
        shape (p, Width, 25.0f, { { 0.0f, 0.35f, 0.0f }, { 0.25f, 0.7f, 0.0f },
                                  { 0.50f, 0.35f, 0.0f }, { 0.75f, 0.7f, 0.0f },
                                  { 1.0f, 0.35f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Sidechain Sky", "Ducking");
        set (p, "rate", 4.0f);
        set (p, "smooth", 2.0f);
        set (p, "filter_cutoff", 1200.0f);
        shape (p, Volume, 95.0f, { { 0.0f, 0.04f, -0.2f }, { 0.035f, 1.0f, 0.4f },
                                   { 0.52f, 0.88f, 0.0f }, { 0.80f, 0.34f, -0.2f },
                                   { 1.0f, 0.04f, 0.0f } });
        shape (p, Filter, 30.0f, { { 0.0f, 0.25f, 0.0f }, { 0.06f, 0.62f, 0.0f },
                                   { 0.55f, 0.55f, 0.0f }, { 1.0f, 0.25f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Air Tremolo", "Tremolo");
        set (p, "rate", 6.0f);
        set (p, "smooth", 12.0f);
        shape (p, Volume, 28.0f, { { 0.0f, 0.35f, 0.0f }, { 0.5f, 1.0f, 0.0f },
                                   { 1.0f, 0.35f, 0.0f } });
        shape (p, Pan, 14.0f, { { 0.0f, 0.55f, 0.0f }, { 0.5f, 0.68f, 0.0f },
                                { 1.0f, 0.55f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Heaven Gate", "Tremolo");
        set (p, "rate", 6.0f);
        set (p, "smooth", 1.0f);
        shape (p, Volume, 100.0f, { { 0.0f, 1.0f, 0.0f, false },
                                    { 0.44f, 1.0f, 0.0f, true },
                                    { 0.46f, 0.0f, -1.0f, true },
                                    { 0.93f, 0.0f, 0.0f, false },
                                    { 0.96f, 1.0f, -1.0f, false },
                                    { 1.0f, 1.0f, 0.0f, false } });
        presets.push_back (p);
    }

    {
        auto p = base ("Pastel Sweep", "Filter");
        set (p, "rate", 1.0f);
        set (p, "filter_cutoff", 700.0f);
        set (p, "filter_reso", 1.4f);
        set (p, "filter_mode", 0.0f);
        shape (p, Filter, 78.0f, { { 0.0f, 0.22f, 0.0f }, { 0.5f, 0.86f, 0.0f },
                                   { 1.0f, 0.22f, 0.0f } });
        shape (p, Volume, 12.0f, { { 0.0f, 0.72f, 0.0f }, { 0.5f, 1.0f, 0.0f },
                                   { 1.0f, 0.72f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Raindrop Pluck", "Filter");
        set (p, "rate", 5.0f);
        set (p, "smooth", 1.5f);
        set (p, "filter_cutoff", 420.0f);
        set (p, "filter_reso", 2.4f);
        set (p, "filter_mode", 0.0f);
        shape (p, Filter, 92.0f, { { 0.0f, 0.94f, 0.0f }, { 0.055f, 0.12f, -0.4f },
                                   { 0.34f, 0.30f, 0.0f }, { 0.68f, 0.52f, 0.0f },
                                   { 1.0f, 0.94f, 0.0f } });
        shape (p, Volume, 22.0f, { { 0.0f, 1.0f, 0.0f }, { 0.06f, 0.45f, 0.0f },
                                   { 0.5f, 0.78f, 0.0f }, { 1.0f, 1.0f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Cumulus Wah", "Filter");
        set (p, "rate", 11.0f);
        set (p, "filter_cutoff", 520.0f);
        set (p, "filter_reso", 3.1f);
        set (p, "filter_mode", 1.0f);
        shape (p, Filter, 72.0f, { { 0.0f, 0.18f, 0.0f }, { 0.25f, 0.82f, 0.0f },
                                   { 0.5f, 0.18f, 0.0f }, { 0.75f, 0.82f, 0.0f },
                                   { 1.0f, 0.18f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Breeze Pan", "Motion");
        set (p, "rate", 3.0f);
        set (p, "smooth", 16.0f);
        shape (p, Pan, 62.0f, { { 0.0f, 0.08f, 0.0f }, { 0.5f, 0.92f, 0.0f },
                                { 1.0f, 0.08f, 0.0f } });
        shape (p, Width, 22.0f, { { 0.0f, 0.62f, 0.0f }, { 0.5f, 0.38f, 0.0f },
                                  { 1.0f, 0.62f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Skyline Width", "Motion");
        set (p, "rate", 2.0f);
        set (p, "smooth", 10.0f);
        shape (p, Width, 88.0f, { { 0.0f, 0.12f, 0.0f }, { 0.5f, 1.0f, 0.0f },
                                  { 1.0f, 0.12f, 0.0f } });
        shape (p, Pan, 16.0f, { { 0.0f, 0.58f, 0.0f }, { 0.5f, 0.42f, 0.0f },
                                { 1.0f, 0.58f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Marshmallow Swell", "Motion");
        set (p, "rate", 0.0f);
        set (p, "smooth", 32.0f);
        set (p, "filter_cutoff", 900.0f);
        shape (p, Volume, 78.0f, { { 0.0f, 0.05f, 0.0f }, { 0.42f, 0.18f, 0.0f },
                                   { 0.82f, 1.0f, 0.4f }, { 1.0f, 0.05f, 0.0f } });
        shape (p, Filter, 26.0f, { { 0.0f, 0.28f, 0.0f }, { 0.75f, 0.7f, 0.0f },
                                   { 1.0f, 0.28f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Thunder Drive", "Texture");
        set (p, "rate", 4.0f);
        set (p, "drive_tone", 62.0f);
        shape (p, Drive, 82.0f, { { 0.0f, 1.0f, 0.0f }, { 0.12f, 0.1f, 0.0f },
                                  { 0.55f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } });
        shape (p, Volume, 28.0f, { { 0.0f, 0.3f, 0.0f }, { 0.15f, 1.0f, 0.0f },
                                   { 0.7f, 0.9f, 0.0f }, { 1.0f, 0.3f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Pixel Rain", "Texture");
        set (p, "rate", 6.0f);
        set (p, "crush_bits", 8.0f);
        set (p, "crush_rate", 6.0f);
        shape (p, Crush, 88.0f, { { 0.0f, 1.0f, 0.0f, true }, { 0.40f, 1.0f, 0.0f, false },
                                  { 0.43f, 0.15f, -1.0f, true }, { 0.78f, 0.15f, 0.0f, false },
                                  { 0.82f, 0.7f, -1.0f, true }, { 1.0f, 0.7f, 0.0f, false } });
        shape (p, Width, 32.0f, { { 0.0f, 0.3f, 0.0f }, { 0.5f, 0.8f, 0.0f },
                                  { 1.0f, 0.3f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Cloudburst", "Signature");
        set (p, "rate", 5.0f);
        set (p, "smooth", 3.0f);
        set (p, "filter_cutoff", 800.0f);
        set (p, "filter_reso", 1.8f);
        set (p, "drive_tone", 78.0f);
        shape (p, Volume, 62.0f, { { 0.0f, 0.12f, 0.0f }, { 0.08f, 1.0f, 0.0f },
                                   { 0.55f, 0.75f, 0.0f }, { 1.0f, 0.12f, 0.0f } });
        shape (p, Filter, 48.0f, { { 0.0f, 0.18f, 0.0f }, { 0.3f, 0.82f, 0.0f },
                                   { 0.7f, 0.36f, 0.0f }, { 1.0f, 0.18f, 0.0f } });
        shape (p, Drive, 36.0f, { { 0.0f, 0.9f, 0.0f }, { 0.2f, 0.1f, 0.0f },
                                  { 0.8f, 0.25f, 0.0f }, { 1.0f, 0.9f, 0.0f } });
        shape (p, Width, 42.0f, { { 0.0f, 0.25f, 0.0f }, { 0.5f, 0.9f, 0.0f },
                                  { 1.0f, 0.25f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Lullaby", "Tremolo");
        set (p, "rate", 10.0f);
        set (p, "smooth", 22.0f);
        set (p, "filter_cutoff", 1600.0f);
        shape (p, Volume, 26.0f, { { 0.0f, 0.38f, 0.0f }, { 0.5f, 1.0f, 0.0f },
                                   { 1.0f, 0.38f, 0.0f } });
        shape (p, Filter, 18.0f, { { 0.0f, 0.4f, 0.0f }, { 0.5f, 0.6f, 0.0f },
                                   { 1.0f, 0.4f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Golden Hour", "Signature");
        set (p, "rate", 3.0f);
        set (p, "smooth", 18.0f);
        set (p, "filter_cutoff", 2200.0f);
        set (p, "filter_reso", 0.8f);
        set (p, "drive_tone", 88.0f);
        shape (p, Filter, 42.0f, { { 0.0f, 0.38f, 0.0f }, { 0.55f, 0.72f, 0.0f },
                                   { 1.0f, 0.38f, 0.0f } });
        shape (p, Drive, 20.0f, { { 0.0f, 0.1f, 0.0f }, { 0.5f, 0.45f, 0.0f },
                                  { 1.0f, 0.1f, 0.0f } });
        shape (p, Width, 34.0f, { { 0.0f, 0.28f, 0.0f }, { 0.5f, 0.76f, 0.0f },
                                  { 1.0f, 0.28f, 0.0f } });
        presets.push_back (p);
    }

    {
        auto p = base ("Static Halo", "Texture");
        set (p, "rate", 5.0f);
        set (p, "crush_bits", 6.0f);
        set (p, "crush_rate", 3.0f);
        shape (p, Crush, 45.0f, { { 0.0f, 0.15f, 0.0f }, { 0.5f, 0.55f, 0.0f },
                                  { 1.0f, 0.15f, 0.0f } });
        shape (p, Width, 58.0f, { { 0.0f, 0.2f, 0.0f }, { 0.3f, 0.9f, 0.0f },
                                  { 0.7f, 0.75f, 0.0f }, { 1.0f, 0.2f, 0.0f } });
        shape (p, Pan, 24.0f, { { 0.0f, 0.35f, 0.0f }, { 0.5f, 0.72f, 0.0f },
                                { 1.0f, 0.35f, 0.0f } });
        presets.push_back (p);
    }

    return presets;
}
}

std::vector<Preset> createFactoryPresets()
{
    static const auto presets = build();
    return presets;
}
}
