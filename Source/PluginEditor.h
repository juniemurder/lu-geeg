#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>
#include <vector>

#include "CloudsLookAndFeel.h"
#include "CurveEditor.h"
#include "ModuleButton.h"
#include "PluginProcessor.h"

class SheetzEditor final : public juce::AudioProcessorEditor,
                           public juce::FileDragAndDropTarget,
                           private juce::Timer
{
public:
    explicit SheetzEditor (SheetzProcessor&);
    ~SheetzEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct PresetEntry
    {
        enum class Kind { Init, Factory, User };

        Kind kind = Kind::Init;
        int index = 0;
        int itemId = 0;
        juce::File file;
        juce::String name;
    };

    void timerCallback() override;

    void setupKnob (Knob&, const juce::String& parameterId, const juce::String& name,
                    const juce::String& suffix, int decimals);
    void selectModule (int module);
    void updateModuleUI();
    void refreshPresetList (const juce::File& selectFile = {});
    void syncPresetSelection();
    void applyEntry (int itemId);
    void stepPreset (int delta);
    void savePresetDialog();
    void importPresets();
    void exportPreset();

    void showStatus (const juce::String& message);
    void resetStatus();

    int findPresetEntryForCurrent() const;
    int findPresetEntryForFile (const juce::File&) const;

    void drawSky (juce::Graphics&);
    void drawCloud (juce::Graphics&, float x, float y, float width, float height, float alpha) const;
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, float alpha) const;

    SheetzProcessor& processor;
    CloudsLookAndFeel lookAndFeel;
    CurveEditor curveEditor;

    std::array<std::unique_ptr<ModuleButton>, sheetz::NumModules> moduleButtons;

    juce::ComboBox presetBox;
    juce::ComboBox rateBox;
    juce::ComboBox filterModeBox;

    juce::TextButton prevButton { "<" };
    juce::TextButton nextButton { ">" };
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton importButton { "IMPORT" };
    juce::TextButton exportButton { "EXPORT" };
    juce::TextButton folderButton { "DIR" };
    juce::TextButton snapButton { "SNAP" };
    juce::TextButton resetButton { "RESET" };
    juce::TextButton flipButton { "FLIP" };
    juce::TextButton onButton { "MODULE ON" };

    Knob depthKnob;
    Knob mixKnob;
    Knob smoothKnob;
    Knob outputKnob;
    Knob phaseKnob;
    Knob cutoffKnob;
    Knob resoKnob;
    Knob toneKnob;
    Knob bitsKnob;
    Knob crushRateKnob;

    juce::Label curveTitleLabel;
    juce::Label moduleTitleLabel;
    juce::Label moduleHintLabel;
    juce::Label presetInfoLabel;
    juce::Label statusLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment;
    std::unique_ptr<juce::FileChooser> fileChooser;

    std::vector<PresetEntry> presetEntries;
    int selectedModule = 0;
    int attachedModule = -1;
    int lastPresetRevision = -1;
    int statusTicks = 0;
    int cloudTick = 0;

    juce::Rectangle<int> curvePanelBounds;
    juce::Rectangle<int> rightPanelBounds;
    juce::Rectangle<int> globalPanelBounds;
    juce::Rectangle<int> moduleRailBounds;

    juce::TooltipWindow tooltipWindow { this, 700 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SheetzEditor)
};
