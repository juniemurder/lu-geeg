#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace juce;

namespace
{
constexpr const char* kStateTag = "SHEETZ_STATE";
constexpr const char* kPresetFormat = "SHEETZ_PRESET";

int moduleIndexFromId (const String& id)
{
    for (int m = 0; m < sheetz::NumModules; ++m)
        if (id == sheetz::moduleId (m))
            return m;
    return -1;
}
}

SheetzProcessor::SheetzProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pRate = apvts.getRawParameterValue ("rate");
    pPhase = apvts.getRawParameterValue ("phase");
    pSmooth = apvts.getRawParameterValue ("smooth");
    pMix = apvts.getRawParameterValue ("mix");
    pOutput = apvts.getRawParameterValue ("output");
    pFilterCutoff = apvts.getRawParameterValue ("filter_cutoff");
    pFilterReso = apvts.getRawParameterValue ("filter_reso");
    pFilterMode = apvts.getRawParameterValue ("filter_mode");
    pDriveTone = apvts.getRawParameterValue ("drive_tone");
    pCrushBits = apvts.getRawParameterValue ("crush_bits");
    pCrushRate = apvts.getRawParameterValue ("crush_rate");

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        const auto id = String (sheetz::moduleId (m));
        pOn[(size_t) m] = apvts.getRawParameterValue (id + "_on");
        pDepth[(size_t) m] = apvts.getRawParameterValue (id + "_depth");
        curves[(size_t) m].reset (sheetz::neutralY (m));
    }
}

AudioProcessorValueTreeState::ParameterLayout SheetzProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "rate", 1 }, "Rate",
        StringArray { "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32",
                      "1/4 dotted", "1/8 dotted", "1/4 triplet", "1/8 triplet" }, 4));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "phase", 1 }, "Phase", NormalisableRange<float> (0.0f, 360.0f, 1.0f), 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "smooth", 1 }, "Smooth", NormalisableRange<float> (0.0f, 50.0f, 0.1f), 4.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output", NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "filter_cutoff", 1 }, "Cutoff",
        NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.32f), 1000.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "filter_reso", 1 }, "Resonance", NormalisableRange<float> (0.1f, 8.0f, 0.01f), 0.8f));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { "filter_mode", 1 }, "Filter Mode",
        StringArray { "Low-pass", "Band-pass", "High-pass" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "drive_tone", 1 }, "Drive Tone", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "crush_bits", 1 }, "Crush Bits", NormalisableRange<float> (2.0f, 16.0f, 1.0f), 12.0f));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { "crush_rate", 1 }, "Crush Downsample", NormalisableRange<float> (1.0f, 32.0f, 1.0f), 4.0f));

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        const auto id = String (sheetz::moduleId (m));
        const auto name = String (sheetz::moduleName (m));

        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { id + "_on", 1 }, name + " On", m == sheetz::Volume));

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id + "_depth", 1 }, name + " Depth",
            NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    }

    return layout;
}

void SheetzProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (uint32) jmax (1, samplesPerBlock);
    spec.numChannels = 2;
    filter.prepare (spec);
    filter.reset();

    driveToneState.fill (0.0f);
    crushHeld.fill (0.0f);
    crushCounter.fill (0);
    freePpq = 0.0;

    for (int m = 0; m < sheetz::NumModules; ++m)
        smoothedMod[(size_t) m] = curves[(size_t) m].read (0.0f);
}

void SheetzProcessor::releaseResources()
{
    filter.reset();
}

bool SheetzProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return (output == AudioChannelSet::stereo() || output == AudioChannelSet::mono())
           && layouts.getMainInputChannelSet() == output;
}

void SheetzProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numChannels = jmin (2, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();

    if (numSamples <= 0 || numChannels <= 0)
        return;

    double bpm = 120.0;
    double ppq = freePpq;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto hostBpm = position->getBpm())
                if (*hostBpm > 1.0)
                    bpm = *hostBpm;

            if (position->getIsPlaying())
                if (auto hostPpq = position->getPpqPosition())
                    ppq = *hostPpq;
        }
    }

    static const double beatsPerCycle[] = {
        16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
        1.5, 0.75, 2.0 / 3.0, 1.0 / 3.0
    };

    const int rateIndex = jlimit (0, 11, (int) pRate->load());
    const double beats = beatsPerCycle[rateIndex];
    const double phaseOffset = (double) pPhase->load() / 360.0;
    const double ppqPerSample = bpm / (60.0 * currentSampleRate);

    const float mix = jlimit (0.0f, 1.0f, pMix->load() * 0.01f);
    const float outputGain = Decibels::decibelsToGain (pOutput->load());
    const float smoothMs = jmax (0.0f, pSmooth->load());
    const float smoothCoefficient = smoothMs <= 0.01f
                                        ? 1.0f
                                        : 1.0f - std::exp (-1.0f / (float) (currentSampleRate * smoothMs / 1000.0));

    auto isOn = [this] (int m) { return pOn[(size_t) m]->load() > 0.5f; };
    auto depthOf = [this] (int m) { return pDepth[(size_t) m]->load() * 0.01f; };

    const bool filterActive = isOn (sheetz::Filter) && depthOf (sheetz::Filter) > 0.0001f;
    const bool volumeActive = isOn (sheetz::Volume) && depthOf (sheetz::Volume) > 0.0001f;
    const bool panActive = isOn (sheetz::Pan) && depthOf (sheetz::Pan) > 0.0001f;
    const bool widthActive = isOn (sheetz::Width) && depthOf (sheetz::Width) > 0.0001f;
    const bool driveActive = isOn (sheetz::Drive) && depthOf (sheetz::Drive) > 0.0001f;
    const bool crushActive = isOn (sheetz::Crush) && depthOf (sheetz::Crush) > 0.0001f;

    const float filterDepth = depthOf (sheetz::Filter);
    const float volumeDepth = depthOf (sheetz::Volume);
    const float panDepth = depthOf (sheetz::Pan);
    const float widthDepth = depthOf (sheetz::Width);
    const float driveDepth = depthOf (sheetz::Drive);
    const float crushDepth = depthOf (sheetz::Crush);

    // Keep the cutoff below Nyquist for low sample-rate hosts too; the TPT
    // filter blows up if the cutoff reaches half the sample rate.
    const float maxCutoff = jmin (20000.0f, (float) (currentSampleRate * 0.49));
    const float baseCutoff = jlimit (20.0f, maxCutoff, pFilterCutoff->load());
    const float baseResonance = jlimit (0.1f, 8.0f, pFilterReso->load());
    const int filterMode = jlimit (0, 2, (int) pFilterMode->load());

    filter.setResonance (baseResonance);
    switch (filterMode)
    {
        case 1:  filter.setType (dsp::StateVariableTPTFilterType::bandpass); break;
        case 2:  filter.setType (dsp::StateVariableTPTFilterType::highpass); break;
        default: filter.setType (dsp::StateVariableTPTFilterType::lowpass);  break;
    }

    const float tone = jlimit (0.0f, 1.0f, pDriveTone->load() * 0.01f);
    const float toneHz = 1000.0f * std::pow (20.0f, tone);
    const float toneCoefficient = 1.0f - std::exp (-MathConstants<float>::twoPi * toneHz / (float) currentSampleRate);

    const float baseBits = jlimit (2.0f, 16.0f, pCrushBits->load());
    const float baseStep = jlimit (1.0f, 32.0f, pCrushRate->load());

    float* channels[2] = { buffer.getWritePointer (0), numChannels > 1 ? buffer.getWritePointer (1) : nullptr };

    float phase = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        double wrapped = std::fmod (ppq / beats + phaseOffset, 1.0);
        if (wrapped < 0.0)
            wrapped += 1.0;
        phase = (float) wrapped;

        float modValues[sheetz::NumModules];

        for (int m = 0; m < sheetz::NumModules; ++m)
        {
            const float value = curves[(size_t) m].read (phase);
            auto& smooth = smoothedMod[(size_t) m];
            smooth += smoothCoefficient * (value - smooth);
            modValues[m] = smooth;
        }

        float left = channels[0][sample];
        float right = numChannels > 1 ? channels[1][sample] : 0.0f;
        const float dryLeft = left;
        const float dryRight = right;

        if (filterActive)
        {
            const float cutoff = jlimit (20.0f, maxCutoff,
                                        baseCutoff * std::exp2 (filterDepth * (modValues[sheetz::Filter] * 2.0f - 1.0f) * 5.0f));
            filter.setCutoffFrequency (cutoff);
            left = filter.processSample (0, left);
            if (numChannels > 1)
                right = filter.processSample (1, right);
        }

        if (driveActive)
        {
            const float amount = driveDepth * modValues[sheetz::Drive];
            if (amount > 0.0005f)
            {
                const float gain = 1.0f + amount * 24.0f;
                const float makeup = 1.0f / std::sqrt (gain);

                left = std::tanh (left * gain) * makeup;
                right = std::tanh (right * gain) * makeup;

                driveToneState[0] += toneCoefficient * (left - driveToneState[0]);
                left = driveToneState[0];

                if (numChannels > 1)
                {
                    driveToneState[1] += toneCoefficient * (right - driveToneState[1]);
                    right = driveToneState[1];
                }
            }
        }

        if (crushActive)
        {
            const float amount = crushDepth * modValues[sheetz::Crush];
            if (amount > 0.0005f)
            {
                const float bits = jlimit (2.0f, 16.0f, 16.0f - (16.0f - baseBits) * amount);
                const float levels = std::pow (2.0f, bits) - 1.0f;
                const float step = 1.0f + (baseStep - 1.0f) * amount;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    if (--crushCounter[(size_t) ch] <= 0)
                    {
                        crushCounter[(size_t) ch] = jmax (1, (int) step);
                        float value = ch == 0 ? left : right;
                        crushHeld[(size_t) ch] = std::round (value * levels) / levels;
                    }

                    if (ch == 0) left = crushHeld[0];
                    else         right = crushHeld[1];
                }
            }
        }

        if (widthActive && numChannels > 1)
        {
            const float width = jlimit (0.0f, 2.0f, 1.0f + widthDepth * (modValues[sheetz::Width] * 2.0f - 1.0f));
            const float mid = (left + right) * 0.5f;
            const float side = (left - right) * 0.5f * width;
            left = mid + side;
            right = mid - side;
        }

        if (panActive)
        {
            const float pan = jlimit (-1.0f, 1.0f, panDepth * (modValues[sheetz::Pan] * 2.0f - 1.0f));

            if (numChannels > 1)
            {
                if (pan > 0.0f) left *= 1.0f - pan;
                else            right *= 1.0f + pan;
            }
        }

        if (volumeActive)
        {
            const float gain = 1.0f - volumeDepth + volumeDepth * modValues[sheetz::Volume];
            left *= gain;
            right *= gain;
        }

        channels[0][sample] = (dryLeft + (left - dryLeft) * mix) * outputGain;

        if (numChannels > 1)
            channels[1][sample] = (dryRight + (right - dryRight) * mix) * outputGain;

        ppq += ppqPerSample;
    }

    freePpq = ppq;
    uiPhase.store (phase);
}

void SheetzProcessor::getStateInformation (MemoryBlock& destData)
{
    auto root = std::make_unique<XmlElement> (kStateTag);
    root->setAttribute ("version", 1);
    root->setAttribute ("presetName", currentPresetName);
    root->setAttribute ("presetCategory", currentPresetCategory);

    if (auto params = apvts.copyState().createXml())
        root->addChildElement (params.release());

    auto* curvesXml = root->createNewChildElement ("CURVES");

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        auto* curveXml = curvesXml->createNewChildElement ("CURVE");
        curveXml->setAttribute ("module", sheetz::moduleId (m));

        for (const auto& point : curves[(size_t) m].getPoints())
        {
            auto* pointXml = curveXml->createNewChildElement ("P");
            pointXml->setAttribute ("x", (double) point.x);
            pointXml->setAttribute ("y", (double) point.y);
            pointXml->setAttribute ("t", (double) point.tension);
            pointXml->setAttribute ("h", point.hold);
        }
    }

    copyXmlToBinary (*root, destData);
}

void SheetzProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (! restoreStateXml (*xml))
            applyLegacyXml (*xml, "Imported");
    }
}

bool SheetzProcessor::restoreStateXml (const XmlElement& xml)
{
    if (! xml.hasTagName (kStateTag))
        return false;

    if (auto* params = xml.getChildByName ("PARAMS"))
    {
        auto tree = ValueTree::fromXml (*params);
        if (tree.isValid())
            apvts.replaceState (tree);
    }

    if (auto* curvesXml = xml.getChildByName ("CURVES"))
    {
        for (auto* curveXml = curvesXml->getFirstChildElement(); curveXml != nullptr;
             curveXml = curveXml->getNextElement())
        {
            const int m = moduleIndexFromId (curveXml->getStringAttribute ("module"));

            if (m < 0)
                continue;

            std::vector<sheetz::CurvePoint> points;

            for (auto* pointXml = curveXml->getFirstChildElement(); pointXml != nullptr;
                 pointXml = pointXml->getNextElement())
            {
                points.push_back ({ (float) pointXml->getDoubleAttribute ("x"),
                                    (float) pointXml->getDoubleAttribute ("y"),
                                    (float) pointXml->getDoubleAttribute ("t"),
                                    pointXml->getBoolAttribute ("h") });
            }

            if (points.size() >= 2)
                curves[(size_t) m].setPoints (std::move (points));
        }
    }

    currentPresetName = xml.getStringAttribute ("presetName", currentPresetName);
    currentPresetCategory = xml.getStringAttribute ("presetCategory", currentPresetCategory);
    currentPresetFile = File();
    presetRevision.fetch_add (1);
    return true;
}

sheetz::Curve& SheetzProcessor::getCurve (int module) noexcept
{
    return curves[(size_t) jlimit (0, sheetz::NumModules - 1, module)];
}

const sheetz::Curve& SheetzProcessor::getCurve (int module) const noexcept
{
    return curves[(size_t) jlimit (0, sheetz::NumModules - 1, module)];
}

void SheetzProcessor::curveEdited (int module)
{
    curves[(size_t) jlimit (0, sheetz::NumModules - 1, module)].bake();
}

void SheetzProcessor::curveChangeFinished()
{
    updateHostDisplay (ChangeDetails().withNonParameterStateChanged (true));
}

void SheetzProcessor::setDefaults()
{
    for (auto* parameter : getParameters())
        parameter->setValueNotifyingHost (parameter->getDefaultValue());
}

void SheetzProcessor::setParamValue (const String& id, float actualValue)
{
    if (auto* parameter = apvts.getParameter (id))
        parameter->setValueNotifyingHost (jlimit (0.0f, 1.0f, parameter->convertTo0to1 (actualValue)));
}

void SheetzProcessor::setCurrentPreset (const String& name, const String& category, const File& file)
{
    currentPresetName = name.isNotEmpty() ? name : "Untitled";
    currentPresetCategory = category.isNotEmpty() ? category : "User";
    currentPresetFile = file;
}

void SheetzProcessor::applyPreset (const sheetz::Preset& preset)
{
    setDefaults();

    for (const auto& entry : preset.params)
        setParamValue (entry.first, entry.second);

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        const auto id = String (sheetz::moduleId (m));
        setParamValue (id + "_on", preset.enabled[(size_t) m] ? 1.0f : 0.0f);
        setParamValue (id + "_depth", preset.depth[(size_t) m]);

        const auto& points = preset.curves[(size_t) m];
        if (points.size() >= 2)
            curves[(size_t) m].setPoints (points);
        else
            curves[(size_t) m].reset (sheetz::neutralY (m));
    }

    setCurrentPreset (preset.name, preset.category, File());
    presetRevision.fetch_add (1);
}

void SheetzProcessor::applyFactoryPreset (int index)
{
    auto factory = getFactoryPresets();
    if (index >= 0 && index < (int) factory.size())
        applyPreset (factory[(size_t) index]);
}

void SheetzProcessor::resetToInit()
{
    setDefaults();

    for (int m = 0; m < sheetz::NumModules; ++m)
        curves[(size_t) m].reset (sheetz::neutralY (m));

    setCurrentPreset ("Init", "Init", File());
    presetRevision.fetch_add (1);
}

File SheetzProcessor::getUserPresetDirectory() const
{
    auto directory = File::getSpecialLocation (File::userDocumentsDirectory)
                         .getChildFile ("SHEETZ!")
                         .getChildFile ("Presets");
    directory.createDirectory();
    return directory;
}

std::vector<File> SheetzProcessor::listUserPresets() const
{
    auto files = getUserPresetDirectory().findChildFiles (File::findFiles, false, "*.sheetz");
    std::vector<File> result;
    result.reserve ((size_t) files.size());

    for (const auto& file : files)
        result.push_back (file);

    std::sort (result.begin(), result.end(),
               [] (const File& a, const File& b) { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });

    return result;
}

var SheetzProcessor::createPresetVar (const String& name, const String& category)
{
    auto* root = new DynamicObject();
    root->setProperty ("format", kPresetFormat);
    root->setProperty ("version", 1);
    root->setProperty ("name", name);
    root->setProperty ("category", category);
    root->setProperty ("author", "SHEETZ!");

    auto* params = new DynamicObject();

    for (auto* parameter : getParameters())
    {
        if (auto* ranged = dynamic_cast<RangedAudioParameter*> (parameter))
        {
            const auto id = ranged->getParameterID();
            params->setProperty (Identifier (id), (double) ranged->convertFrom0to1 (ranged->getValue()));
        }
    }

    root->setProperty ("params", var (params));

    auto* curvesObject = new DynamicObject();

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        Array<var> points;

        for (const auto& point : curves[(size_t) m].getPoints())
        {
            auto* object = new DynamicObject();
            object->setProperty ("x", (double) point.x);
            object->setProperty ("y", (double) point.y);
            object->setProperty ("t", (double) point.tension);
            object->setProperty ("hold", point.hold);
            points.add (var (object));
        }

        curvesObject->setProperty (Identifier (sheetz::moduleId (m)), var (points));
    }

    root->setProperty ("curves", var (curvesObject));
    return var (root);
}

bool SheetzProcessor::applyPresetVar (const var& preset, String* errorMessage)
{
    auto* root = preset.getDynamicObject();

    if (root == nullptr)
    {
        if (errorMessage != nullptr)
            *errorMessage = "Preset is not a JSON object.";
        return false;
    }

    const auto format = root->getProperty ("format").toString();

    if (format.isNotEmpty() && format != kPresetFormat)
    {
        if (errorMessage != nullptr)
            *errorMessage = "Not a SHEETZ! preset.";
        return false;
    }

    auto paramsVar = root->getProperty ("params");

    if (auto* params = paramsVar.getDynamicObject())
    {
        setDefaults();
        const auto& properties = params->getProperties();

        for (int i = 0; i < properties.size(); ++i)
            setParamValue (properties.getName (i).toString(), (float) (double) properties.getValueAt (i));
    }
    else
    {
        if (errorMessage != nullptr)
            *errorMessage = "Preset has no params object.";
        return false;
    }

    for (int m = 0; m < sheetz::NumModules; ++m)
        curves[(size_t) m].reset (sheetz::neutralY (m));

    auto curvesVar = root->getProperty ("curves");

    if (auto* curvesObject = curvesVar.getDynamicObject())
    {
        for (int m = 0; m < sheetz::NumModules; ++m)
        {
            auto curveVar = curvesObject->getProperty (Identifier (sheetz::moduleId (m)));

            if (! curveVar.isArray())
                continue;

            std::vector<sheetz::CurvePoint> points;
            auto* array = curveVar.getArray();

            for (const auto& pointVar : *array)
            {
                if (auto* object = pointVar.getDynamicObject())
                    points.push_back ({ (float) (double) object->getProperty ("x"),
                                        (float) (double) object->getProperty ("y"),
                                        (float) (double) object->getProperty ("t"),
                                        (bool) object->getProperty ("hold") });
            }

            if (points.size() >= 2)
                curves[(size_t) m].setPoints (std::move (points));
        }
    }

    setCurrentPreset (root->getProperty ("name").toString(), root->getProperty ("category").toString(), File());
    presetRevision.fetch_add (1);
    return true;
}

bool SheetzProcessor::savePresetFile (const File& file, const String& name, const String& category)
{
    auto target = file.withFileExtension ("sheetz");
    target.getParentDirectory().createDirectory();

    auto json = JSON::toString (createPresetVar (name, category), false);

    if (! target.replaceWithText (json))
        return false;

    setCurrentPreset (name, category, target);
    presetRevision.fetch_add (1);
    return true;
}

bool SheetzProcessor::exportPresetFile (const File& file, const String& name, const String& category)
{
    auto target = file.withFileExtension ("sheetz");
    target.getParentDirectory().createDirectory();

    auto json = JSON::toString (createPresetVar (name, category), false);

    if (! target.replaceWithText (json))
        return false;

    presetRevision.fetch_add (1);
    return true;
}

bool SheetzProcessor::loadPresetFile (const File& file)
{
    if (! file.existsAsFile())
        return false;

    const auto text = file.loadFileAsString();
    var parsed;
    const auto parseResult = JSON::parse (text, parsed);
    String error;

    if (parseResult.wasOk() && applyPresetVar (parsed, &error))
    {
        currentPresetFile = file;
        presetRevision.fetch_add (1);
        return true;
    }

    if (auto xml = XmlDocument::parse (text))
        if (applyLegacyXml (*xml, file.getFileNameWithoutExtension()))
        {
            currentPresetFile = file;
            presetRevision.fetch_add (1);
            return true;
        }

    return false;
}

File SheetzProcessor::importPresetFile (const File& file)
{
    if (! loadPresetFile (file))
        return {};

    auto directory = getUserPresetDirectory();
    File destination;

    if (file.isAChildOf (directory))
    {
        destination = file;
    }
    else
    {
        const auto safeName = File::createLegalFileName (getPresetName());
        destination = directory.getChildFile (safeName + ".sheetz");

        if (destination.existsAsFile())
            destination = directory.getNonexistentChildFile (safeName, "sheetz", true);

        // Always rewrite imported presets as canonical JSON. This converts
        // legacy .shz/XML files and keeps the user preset folder readable.
        if (! savePresetFile (destination, getPresetName(), getPresetCategory()))
            return {};

        destination = getCurrentPresetFile();
    }

    currentPresetFile = destination;
    presetRevision.fetch_add (1);
    return destination;
}

bool SheetzProcessor::applyLegacyXml (const XmlElement& xml, const String& fallbackName)
{
    if (! xml.hasTagName ("SheetzPreset"))
        return false;

    setDefaults();

    for (int m = 0; m < sheetz::NumModules; ++m)
        curves[(size_t) m].reset (sheetz::neutralY (m));

    auto applyParam = [this] (const String& id, float value, bool stretchOldRange)
    {
        if (id == "rate")
        {
            setParamValue ("rate", value);
        }
        else if (id == "phase")
        {
            setParamValue ("phase", stretchOldRange ? value * 360.0f : value);
        }
        else if (id == "mix")
        {
            setParamValue ("mix", stretchOldRange ? value * 100.0f : value);
        }
        else if (id == "vol")
        {
            setParamValue ("volume_on", 1.0f);
            setParamValue ("volume_depth", value * 100.0f);
        }
        else if (id == "pan")
        {
            setParamValue ("pan_on", 1.0f);
            setParamValue ("pan_depth", std::abs (value) * 100.0f);
        }
        else if (id == "filt")
        {
            setParamValue ("filter_on", 1.0f);
            setParamValue ("filter_depth", value * 100.0f);
        }
        else if (id == "cutoff")
        {
            setParamValue ("filter_cutoff", value);
        }
        else if (id == "reso")
        {
            setParamValue ("filter_reso", value);
        }
        else if (id == "mode")
        {
            setParamValue ("filter_mode", value);
        }
        else
        {
            // Also accept already-modern IDs such as volume_depth or smooth.
            setParamValue (id, value);
        }
    };

    auto applyAttributes = [&applyParam] (const XmlElement& element, bool stretchOldRange)
    {
        for (int i = 0; i < element.getNumAttributes(); ++i)
            applyParam (element.getAttributeName (i),
                        element.getAttributeValue (i).getFloatValue(),
                        stretchOldRange);
    };

    // The original MVP stored parameter values as attributes on the root node.
    applyAttributes (xml, true);

    if (auto* params = xml.getChildByName ("PARAMS"))
        applyAttributes (*params, false);

    for (auto* child = xml.getFirstChildElement(); child != nullptr; child = child->getNextElement())
        if (child->hasTagName ("PARAM"))
            applyParam (child->getStringAttribute ("id"),
                        (float) child->getDoubleAttribute ("value"), true);

    std::vector<sheetz::CurvePoint> oldCurve;

    if (auto* curveXml = xml.getChildByName ("CURVE"))
    {
        for (auto* pointXml = curveXml->getFirstChildElement(); pointXml != nullptr;
             pointXml = pointXml->getNextElement())
        {
            oldCurve.push_back ({ (float) pointXml->getDoubleAttribute ("x"),
                                  (float) pointXml->getDoubleAttribute ("y"),
                                  (float) pointXml->getDoubleAttribute ("t"),
                                  false });
        }
    }

    if (oldCurve.size() >= 2)
        for (int m : { sheetz::Volume, sheetz::Filter, sheetz::Pan })
            curves[(size_t) m].setPoints (oldCurve);

    // The original plugin used one shared curve. If a modern depth value was
    // supplied instead of the old vol/pan/filt names, make sure it is audible.
    if (pDepth[(size_t) sheetz::Volume]->load() > 0.001f)
        setParamValue ("volume_on", 1.0f);

    if (pDepth[(size_t) sheetz::Filter]->load() > 0.001f)
        setParamValue ("filter_on", 1.0f);

    if (pDepth[(size_t) sheetz::Pan]->load() > 0.001f)
        setParamValue ("pan_on", 1.0f);

    setCurrentPreset (fallbackName.isNotEmpty() ? fallbackName : "Imported", "Legacy", File());
    presetRevision.fetch_add (1);
    return true;
}

juce::AudioProcessorEditor* SheetzProcessor::createEditor()
{
    return new SheetzEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SheetzProcessor();
}
