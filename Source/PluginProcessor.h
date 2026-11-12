#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <vector>

#include "Curve.h"
#include "FactoryPresets.h"
#include "Modules.h"

class SheetzProcessor final : public juce::AudioProcessor
{
public:
    SheetzProcessor();
    ~SheetzProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SHEETZ!"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    sheetz::Curve& getCurve (int module) noexcept;
    const sheetz::Curve& getCurve (int module) const noexcept;
    void curveEdited (int module);

    /** Call from the message thread after a curve edit so the host knows that
        non-parameter state has changed and can mark the session as dirty.
    */
    void curveChangeFinished();

    float getCurrentPhase() const noexcept { return uiPhase.load(); }
    int getPresetRevision() const noexcept { return presetRevision.load(); }

    juce::String getPresetName() const { return currentPresetName; }
    juce::String getPresetCategory() const { return currentPresetCategory; }
    juce::File getCurrentPresetFile() const { return currentPresetFile; }

    std::vector<sheetz::Preset> getFactoryPresets() const { return sheetz::createFactoryPresets(); }
    void applyFactoryPreset (int index);
    void resetToInit();

    juce::File getUserPresetDirectory() const;
    std::vector<juce::File> listUserPresets() const;

    juce::var createPresetVar (const juce::String& name, const juce::String& category);
    bool applyPresetVar (const juce::var& preset, juce::String* errorMessage = nullptr);

    bool savePresetFile (const juce::File& file, const juce::String& name, const juce::String& category);
    bool loadPresetFile (const juce::File& file);
    juce::File importPresetFile (const juce::File& file);
    bool exportPresetFile (const juce::File& file, const juce::String& name, const juce::String& category);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setDefaults();
    void setParamValue (const juce::String& id, float actualValue);
    void setCurrentPreset (const juce::String& name, const juce::String& category, const juce::File& file);
    void applyPreset (const sheetz::Preset& preset);
    bool applyLegacyXml (const juce::XmlElement& xml, const juce::String& fallbackName);
    bool restoreStateXml (const juce::XmlElement& xml);

    std::array<sheetz::Curve, sheetz::NumModules> curves;
    std::atomic<float> uiPhase { 0.0f };
    std::atomic<int> presetRevision { 0 };

    std::atomic<float>* pRate = nullptr;
    std::atomic<float>* pPhase = nullptr;
    std::atomic<float>* pSmooth = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso = nullptr;
    std::atomic<float>* pFilterMode = nullptr;
    std::atomic<float>* pDriveTone = nullptr;
    std::atomic<float>* pCrushBits = nullptr;
    std::atomic<float>* pCrushRate = nullptr;
    std::array<std::atomic<float>*, sheetz::NumModules> pOn {};
    std::array<std::atomic<float>*, sheetz::NumModules> pDepth {};

    juce::dsp::StateVariableTPTFilter<float> filter;
    std::array<float, sheetz::NumModules> smoothedMod {};
    std::array<float, 2> driveToneState {};
    std::array<float, 2> crushHeld {};
    std::array<int, 2> crushCounter {};

    double currentSampleRate = 44100.0;
    double freePpq = 0.0;

    juce::String currentPresetName { "Init" };
    juce::String currentPresetCategory { "Init" };
    juce::File currentPresetFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SheetzProcessor)
};
