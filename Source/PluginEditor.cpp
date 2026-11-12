#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

using namespace juce;

namespace
{
constexpr int kStatusTicks = 240;

int extensionIsPreset (const String& path)
{
    const auto extension = File (path).getFileExtension().toLowerCase();
    return extension == ".sheetz" || extension == ".json" || extension == ".shz" || extension == ".xml";
}
}

SheetzEditor::SheetzEditor (SheetzProcessor& p)
    : AudioProcessorEditor (&p), processor (p), curveEditor (p)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (900, 600, 1600, 1000);

    addAndMakeVisible (curveEditor);

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        moduleButtons[(size_t) m] = std::make_unique<ModuleButton> (processor, m);
        addAndMakeVisible (*moduleButtons[(size_t) m]);
        moduleButtons[(size_t) m]->onSelect = [this] (int module) { selectModule (module); };
    }

    addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("Choose a preset");
    presetBox.setTooltip ("Factory and user presets. User presets live in Documents/SHEETZ!/Presets.");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();

        if (id > 0)
            applyEntry (id);
    };

    addAndMakeVisible (rateBox);
    rateBox.setTooltip ("Tempo-synced cycle length");
    rateBox.addItemList (processor.apvts.getParameter ("rate")->getAllValueStrings(), 1);
    rateAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "rate", rateBox);

    addAndMakeVisible (filterModeBox);
    filterModeBox.setTooltip ("Filter character");
    filterModeBox.addItemList (processor.apvts.getParameter ("filter_mode")->getAllValueStrings(), 1);
    filterModeAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, "filter_mode", filterModeBox);

    auto addPillButton = [this] (TextButton& button, const String& tooltipText)
    {
        addAndMakeVisible (button);
        button.setTooltip (tooltipText);
        button.setColour (TextButton::buttonColourId, Colour (0xD9FFFFFF));
        button.setColour (TextButton::textColourOffId, CloudsLookAndFeel::ink());
    };

    addPillButton (prevButton, "Previous preset");
    addPillButton (nextButton, "Next preset");
    addPillButton (saveButton, "Save the current sound as a user preset");
    addPillButton (importButton, "Import .sheetz, .json, or legacy .shz presets");
    addPillButton (exportButton, "Export the current sound as a .sheetz file");
    addPillButton (folderButton, "Open the user preset folder");
    addPillButton (snapButton, "Snap points to the beat grid");
    addPillButton (resetButton, "Reset this curve to its neutral shape");
    addPillButton (flipButton, "Flip this curve upside down");
    addPillButton (onButton, "Enable or bypass this shaper");

    snapButton.setClickingTogglesState (true);
    snapButton.setToggleState (true, dontSendNotification);
    snapButton.onClick = [this] { curveEditor.setSnap (snapButton.getToggleState()); };
    resetButton.onClick = [this] { curveEditor.resetCurve(); showStatus ("Curve reset"); };
    flipButton.onClick = [this] { curveEditor.flipCurve(); showStatus ("Curve flipped"); };
    onButton.setClickingTogglesState (true);
    saveButton.onClick = [this] { savePresetDialog(); };
    importButton.onClick = [this] { importPresets(); };
    exportButton.onClick = [this] { exportPreset(); };
    folderButton.onClick = [this] { processor.getUserPresetDirectory().revealToUser(); };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };

    setupKnob (depthKnob, "volume_depth", "Depth", " %", 0);
    setupKnob (mixKnob, "mix", "Mix", " %", 0);
    setupKnob (smoothKnob, "smooth", "Smooth", " ms", 1);
    setupKnob (outputKnob, "output", "Output", " dB", 1);
    setupKnob (phaseKnob, "phase", "Phase", " deg", 0);
    setupKnob (cutoffKnob, "filter_cutoff", "Cutoff", " Hz", 0);
    setupKnob (resoKnob, "filter_reso", "Reso", "", 2);
    setupKnob (toneKnob, "drive_tone", "Tone", " %", 0);
    setupKnob (bitsKnob, "crush_bits", "Bits", " bit", 0);
    setupKnob (crushRateKnob, "crush_rate", "Rate", "x", 0);

    curveTitleLabel.setFont (Font (FontOptions (15.0f).withStyle ("Bold")));
    curveTitleLabel.setColour (Label::textColourId, CloudsLookAndFeel::ink());
    curveTitleLabel.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (curveTitleLabel);

    moduleTitleLabel.setFont (Font (FontOptions (12.0f).withStyle ("Bold")));
    moduleTitleLabel.setColour (Label::textColourId, CloudsLookAndFeel::ink().withAlpha (0.9f));
    moduleTitleLabel.setJustificationType (Justification::centredLeft);
    addAndMakeVisible (moduleTitleLabel);

    moduleHintLabel.setFont (Font (FontOptions (10.0f)));
    moduleHintLabel.setColour (Label::textColourId, CloudsLookAndFeel::ink().withAlpha (0.62f));
    moduleHintLabel.setJustificationType (Justification::topLeft);
    moduleHintLabel.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (moduleHintLabel);

    presetInfoLabel.setFont (Font (FontOptions (9.5f).withStyle ("Bold")));
    presetInfoLabel.setColour (Label::textColourId, CloudsLookAndFeel::ink().withAlpha (0.55f));
    presetInfoLabel.setJustificationType (Justification::centred);
    addAndMakeVisible (presetInfoLabel);

    statusLabel.setFont (Font (FontOptions (10.0f)));
    statusLabel.setColour (Label::textColourId, CloudsLookAndFeel::ink().withAlpha (0.58f));
    statusLabel.setJustificationType (Justification::centred);
    statusLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (statusLabel);

    selectModule (0);
    refreshPresetList();
    resetStatus();

    lastPresetRevision = processor.getPresetRevision();
    startTimerHz (30);

    setSize (1040, 680);
}

SheetzEditor::~SheetzEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void SheetzEditor::setupKnob (Knob& knob, const String& parameterId, const String& name,
                              const String& suffix, int decimals)
{
    knob.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    knob.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 66, 15);
    knob.slider.setNumDecimalPlacesToDisplay (decimals);
    knob.slider.setTextValueSuffix (suffix);
    knob.slider.setColour (Slider::rotarySliderFillColourId, CloudsLookAndFeel::accent());
    knob.slider.setTooltip (name);
    knob.slider.setOpaque (false);

    knob.label.setText (name, dontSendNotification);
    knob.label.setFont (Font (FontOptions (9.5f).withStyle ("Bold")));
    knob.label.setJustificationType (Justification::centred);
    knob.label.setColour (Label::textColourId, CloudsLookAndFeel::ink().withAlpha (0.62f));
    knob.label.setOpaque (false);

    addAndMakeVisible (knob.slider);
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, parameterId, knob.slider);
}

void SheetzEditor::selectModule (int module)
{
    selectedModule = jlimit (0, sheetz::NumModules - 1, module);

    for (int m = 0; m < sheetz::NumModules; ++m)
        moduleButtons[(size_t) m]->setSelected (m == selectedModule);

    curveEditor.setModule (selectedModule);
    updateModuleUI();
}

void SheetzEditor::updateModuleUI()
{
    const auto accent = CloudsLookAndFeel::moduleAccent (selectedModule);
    const auto id = String (sheetz::moduleId (selectedModule));
    const bool moduleChanged = attachedModule != selectedModule;

    if (moduleChanged)
    {
        depthKnob.attachment.reset();
        depthKnob.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (
            processor.apvts, id + "_depth", depthKnob.slider);

        onAttachment.reset();
        onAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (
            processor.apvts, id + "_on", onButton);

        attachedModule = selectedModule;
    }

    depthKnob.slider.setColour (Slider::rotarySliderFillColourId, accent);
    onButton.setColour (TextButton::buttonOnColourId, accent);

    curveTitleLabel.setText (String (sheetz::moduleName (selectedModule)).toUpperCase() + " SHAPER",
                             dontSendNotification);
    moduleTitleLabel.setText ("SHAPE  /  " + String (sheetz::moduleName (selectedModule)).toUpperCase(),
                              dontSendNotification);

    switch (selectedModule)
    {
        case sheetz::Volume:
            moduleHintLabel.setText ("Draw gain. Depth sets how far it ducks.",
                                     dontSendNotification);
            break;
        case sheetz::Filter:
            moduleHintLabel.setText ("Cutoff = centre. Sweeps up to 5 octaves.",
                                     dontSendNotification);
            break;
        case sheetz::Pan:
            moduleHintLabel.setText ("Draw stereo position. Centre = unpanned.",
                                     dontSendNotification);
            break;
        case sheetz::Width:
            moduleHintLabel.setText ("Push the sides wide, or fold to mono.",
                                     dontSendNotification);
            break;
        case sheetz::Drive:
            moduleHintLabel.setText ("Curve drives saturation. Tone softens.",
                                     dontSendNotification);
            break;
        case sheetz::Crush:
            moduleHintLabel.setText ("Curve pushes bit and rate reduction.",
                                     dontSendNotification);
            break;
        default:
            break;
    }

    const bool showFilter = selectedModule == sheetz::Filter;
    const bool showDrive = selectedModule == sheetz::Drive;
    const bool showCrush = selectedModule == sheetz::Crush;

    cutoffKnob.slider.setVisible (showFilter);
    cutoffKnob.label.setVisible (showFilter);
    resoKnob.slider.setVisible (showFilter);
    resoKnob.label.setVisible (showFilter);
    filterModeBox.setVisible (showFilter);

    toneKnob.slider.setVisible (showDrive);
    toneKnob.label.setVisible (showDrive);

    bitsKnob.slider.setVisible (showCrush);
    bitsKnob.label.setVisible (showCrush);
    crushRateKnob.slider.setVisible (showCrush);
    crushRateKnob.label.setVisible (showCrush);

    const auto onText = onButton.getToggleState() ? "MODULE ON" : "MODULE OFF";

    if (onButton.getButtonText() != onText)
        onButton.setButtonText (onText);

    if (moduleChanged)
        resized();
}

void SheetzEditor::timerCallback()
{
    const int revision = processor.getPresetRevision();

    if (revision != lastPresetRevision)
    {
        lastPresetRevision = revision;
        syncPresetSelection();
        updateModuleUI();
        curveEditor.repaint();
    }

    const auto onText = onButton.getToggleState() ? "MODULE ON" : "MODULE OFF";

    if (onButton.getButtonText() != onText)
        onButton.setButtonText (onText);

    if (statusTicks > 0)
    {
        --statusTicks;

        if (statusTicks == 0)
            resetStatus();
    }

    if (++cloudTick >= 3)
    {
        cloudTick = 0;
        repaint();
    }
}

void SheetzEditor::refreshPresetList (const File& selectFile)
{
    presetEntries.clear();
    presetBox.clear (dontSendNotification);

    auto addEntry = [this] (PresetEntry::Kind kind, int index, int itemId, const File& file, const String& name)
    {
        presetEntries.push_back ({ kind, index, itemId, file, name });
        presetBox.addItem (name, itemId);
    };

    presetBox.addItem ("Init", 1);
    presetEntries.push_back ({ PresetEntry::Kind::Init, 0, 1, File(), "Init" });

    presetBox.addSectionHeading ("FACTORY");
    const auto factory = processor.getFactoryPresets();
    int itemId = 100;

    for (int i = 0; i < (int) factory.size(); ++i)
        addEntry (PresetEntry::Kind::Factory, i, itemId++, File(), factory[(size_t) i].name);

    presetBox.addSectionHeading ("USER");
    itemId = 1000;

    for (const auto& file : processor.listUserPresets())
        addEntry (PresetEntry::Kind::User, 0, itemId++, file, file.getFileNameWithoutExtension());

    int selected = 1;

    if (selectFile != File())
        selected = findPresetEntryForFile (selectFile);

    if (selected <= 0)
        selected = findPresetEntryForCurrent();

    if (selected <= 0)
        selected = 1;

    presetBox.setSelectedId (selected, dontSendNotification);
    syncPresetSelection();
}

int SheetzEditor::findPresetEntryForFile (const File& file) const
{
    for (const auto& entry : presetEntries)
        if (entry.kind == PresetEntry::Kind::User && entry.file == file)
            return entry.itemId;

    return 0;
}

int SheetzEditor::findPresetEntryForCurrent() const
{
    const auto currentFile = processor.getCurrentPresetFile();

    if (currentFile != File())
        if (const int id = findPresetEntryForFile (currentFile); id > 0)
            return id;

    const auto name = processor.getPresetName();

    if (name == "Init")
        return 1;

    for (const auto& entry : presetEntries)
        if (entry.kind == PresetEntry::Kind::Factory && entry.name == name)
            return entry.itemId;

    for (const auto& entry : presetEntries)
        if (entry.kind == PresetEntry::Kind::User && entry.name == name)
            return entry.itemId;

    return 0;
}

void SheetzEditor::syncPresetSelection()
{
    const int id = findPresetEntryForCurrent();

    if (id > 0)
        presetBox.setSelectedId (id, dontSendNotification);

    presetInfoLabel.setText (processor.getPresetCategory() + "  /  " + processor.getPresetName(),
                             dontSendNotification);
}

void SheetzEditor::applyEntry (int itemId)
{
    auto entry = std::find_if (presetEntries.begin(), presetEntries.end(),
                               [itemId] (const PresetEntry& e) { return e.itemId == itemId; });

    if (entry == presetEntries.end())
        return;

    bool loaded = true;

    switch (entry->kind)
    {
        case PresetEntry::Kind::Init:
            processor.resetToInit();
            break;
        case PresetEntry::Kind::Factory:
            processor.applyFactoryPreset (entry->index);
            break;
        case PresetEntry::Kind::User:
            loaded = processor.loadPresetFile (entry->file);
            break;
    }

    if (! loaded)
    {
        showStatus ("Could not load that preset");
        refreshPresetList();
        return;
    }

    lastPresetRevision = processor.getPresetRevision();

    // Avoid rebuilding the ComboBox from inside its own onChange callback.
    // The list is unchanged when selecting an existing entry; only the
    // selected item and the info label need to move.
    syncPresetSelection();
    updateModuleUI();
    curveEditor.repaint();
    showStatus ("Loaded  " + processor.getPresetName());
}

void SheetzEditor::stepPreset (int delta)
{
    if (presetEntries.empty())
        return;

    const int currentId = presetBox.getSelectedId();
    int currentIndex = 0;

    for (int i = 0; i < (int) presetEntries.size(); ++i)
        if (presetEntries[(size_t) i].itemId == currentId)
        {
            currentIndex = i;
            break;
        }

    const int next = ((currentIndex + delta) % (int) presetEntries.size()
                      + (int) presetEntries.size()) % (int) presetEntries.size();

    presetBox.setSelectedId (presetEntries[(size_t) next].itemId, sendNotificationSync);
}

void SheetzEditor::savePresetDialog()
{
    auto* window = new AlertWindow ("Save preset", "Name this weather system.", MessageBoxIconType::NoIcon, this);
    window->setColour (AlertWindow::backgroundColourId, Colour (0xFFF4FCFF));
    window->setColour (AlertWindow::textColourId, CloudsLookAndFeel::ink());
    window->setColour (AlertWindow::outlineColourId, Colour (0x332A6688));
    const auto suggested = processor.getPresetName() == "Init" ? "My Cloud" : processor.getPresetName();
    window->addTextEditor ("name", suggested);
    window->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    window->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));

    Component::SafePointer<SheetzEditor> safe (this);

    window->enterModalState (true, ModalCallbackFunction::create ([safe, window] (int result)
    {
        if (result != 1 || safe == nullptr)
            return;

        auto name = window->getTextEditorContents ("name").trim();

        if (name.isEmpty())
            name = "My Cloud";

        const auto file = safe->processor.getUserPresetDirectory()
                              .getChildFile (File::createLegalFileName (name) + ".sheetz");

        if (safe->processor.savePresetFile (file, name, "User"))
        {
            safe->refreshPresetList (file);
            safe->lastPresetRevision = safe->processor.getPresetRevision();
            safe->showStatus ("Saved  " + name);
        }
        else
        {
            safe->showStatus ("Could not save preset");
        }
    }), true);
}

void SheetzEditor::importPresets()
{
    fileChooser = std::make_unique<FileChooser> (
        "Import SHEETZ! presets",
        processor.getUserPresetDirectory(),
        "*.sheetz;*.json;*.shz;*.xml");

    Component::SafePointer<SheetzEditor> safe (this);

    fileChooser->launchAsync (FileBrowserComponent::openMode
                                  | FileBrowserComponent::canSelectFiles
                                  | FileBrowserComponent::canSelectMultipleItems,
                              [safe] (const FileChooser& chooser)
    {
        if (safe == nullptr)
            return;

        const auto results = chooser.getResults();
        int imported = 0;
        File lastFile;

        for (const auto& file : results)
        {
            const auto destination = safe->processor.importPresetFile (file);

            if (destination != File())
            {
                ++imported;
                lastFile = destination;
            }
        }

        if (imported > 0)
        {
            safe->lastPresetRevision = safe->processor.getPresetRevision();
            safe->refreshPresetList (lastFile);
            safe->updateModuleUI();
            safe->curveEditor.repaint();
            safe->showStatus ("Imported  " + String (imported) + " preset(s)");
        }
        else
        {
            safe->showStatus ("No valid presets found");
        }
    });
}

void SheetzEditor::exportPreset()
{
    auto name = processor.getPresetName();

    if (name.isEmpty() || name == "Init")
        name = "SHEETZ Preset";

    const auto start = File::getSpecialLocation (File::userDesktopDirectory)
                           .getChildFile (File::createLegalFileName (name) + ".sheetz");

    fileChooser = std::make_unique<FileChooser> ("Export SHEETZ! preset", start, "*.sheetz");
    Component::SafePointer<SheetzEditor> safe (this);

    fileChooser->launchAsync (FileBrowserComponent::saveMode
                                  | FileBrowserComponent::canSelectFiles
                                  | FileBrowserComponent::warnAboutOverwriting,
                              [safe, name] (const FileChooser& chooser)
    {
        if (safe == nullptr)
            return;

        const auto file = chooser.getResult();

        if (file == File())
            return;

        if (safe->processor.exportPresetFile (file, name, safe->processor.getPresetCategory()))
            safe->showStatus ("Exported  " + file.getFileName());
        else
            safe->showStatus ("Export failed");
    });
}

void SheetzEditor::showStatus (const String& message)
{
    statusLabel.setText (message, dontSendNotification);
    statusLabel.setColour (Label::textColourId, CloudsLookAndFeel::accent().darker (0.25f));
    statusTicks = kStatusTicks;
}

void SheetzEditor::resetStatus()
{
    statusLabel.setText ("DRAW THE CURVE  |  double-click a point to delete  |  shift-click for a hold step  |  right-drag for tension",
                         dontSendNotification);
    statusLabel.setColour (Label::textColourId, CloudsLookAndFeel::ink().withAlpha (0.58f));
}

void SheetzEditor::paint (Graphics& g)
{
    drawSky (g);

    const auto ink = CloudsLookAndFeel::ink();
    const auto accent = CloudsLookAndFeel::accent();

    drawPanel (g, curvePanelBounds, 0.16f);
    drawPanel (g, rightPanelBounds, 0.24f);
    drawPanel (g, globalPanelBounds, 0.24f);

    g.setFont (Font (FontOptions (9.0f).withStyle ("Bold")));
    g.setColour (ink.withAlpha (0.48f));

    auto railHeader = moduleRailBounds;
    g.drawText ("SHAPERS", railHeader.removeFromTop (18), Justification::centredLeft, false);

    Path logo;
    logo.addEllipse (23.0f, 22.0f, 17.0f, 16.0f);
    logo.addEllipse (31.0f, 14.0f, 20.0f, 20.0f);
    logo.addEllipse (43.0f, 23.0f, 16.0f, 15.0f);
    logo.addRoundedRectangle (25.0f, 30.0f, 33.0f, 9.0f, 4.5f);

    g.setColour (Colour (0x66FFFFFF));
    g.fillPath (logo);
    g.setColour (accent.withAlpha (0.7f));
    g.strokePath (logo, PathStrokeType (1.1f));

    const Font titleFont (FontOptions (28.0f).withStyle ("Bold"));
    g.setFont (titleFont);
    g.setColour (ink);
    g.drawText ("SHEETZ", Rectangle<int> (68, 6, 190, 34), Justification::centredLeft, false);

    g.setColour (accent);
    const int titleWidth = (int) std::ceil (GlyphArrangement::getStringWidth (titleFont, "SHEETZ"));
    g.drawText ("!", Rectangle<int> (68 + titleWidth, 6, 28, 34),
                Justification::centredLeft, false);

    g.setFont (Font (FontOptions (8.5f).withStyle ("Bold")));
    g.setColour (ink.withAlpha (0.46f));
    g.drawText ("CLOUD MODULATION", Rectangle<int> (70, 39, 180, 12),
                Justification::centredLeft, false);

    g.setFont (Font (FontOptions (8.5f)));
    g.setColour (ink.withAlpha (0.35f));
    g.drawText ("v1.0", Rectangle<int> (getWidth() - 52, getHeight() - 18, 38, 12),
                Justification::centredRight, false);
}

void SheetzEditor::drawSky (Graphics& g)
{
    const auto width = (float) getWidth();
    const auto height = (float) getHeight();

    ColourGradient sky (CloudsLookAndFeel::skyTop(), 0.0f, 0.0f,
                        CloudsLookAndFeel::skyBottom(), 0.0f, height, false);
    g.setGradientFill (sky);
    g.fillAll();

    const float sunX = width - 150.0f;
    const float sunY = 92.0f;
    ColourGradient glow (Colour (0x66FFE6A8), sunX, sunY,
                         Colour (0x00FFE6A8), sunX + 170.0f, sunY + 140.0f, true);
    g.setGradientFill (glow);
    g.fillEllipse (sunX - 170.0f, sunY - 150.0f, 340.0f, 300.0f);

    g.setColour (Colour (0x66FFF2C4));
    g.fillEllipse (sunX - 30.0f, sunY - 30.0f, 60.0f, 60.0f);

    const float time = (float) (Time::getMillisecondCounterHiRes() * 0.001);

    struct Cloud
    {
        float base;
        float y;
        float scale;
        float speed;
        float alpha;
    };

    static const Cloud clouds[] = {
        { 0.04f, 0.10f, 1.4f, 4.0f, 0.28f },
        { 0.62f, 0.05f, 1.0f, 2.4f, 0.20f },
        { 0.30f, 0.21f, 1.9f, 3.1f, 0.15f },
        { 0.82f, 0.34f, 1.2f, 5.0f, 0.17f },
        { 0.08f, 0.56f, 2.1f, 1.9f, 0.12f },
        { 0.55f, 0.66f, 1.7f, 3.4f, 0.13f },
        { 0.23f, 0.80f, 2.5f, 1.4f, 0.19f },
        { 0.72f, 0.91f, 2.9f, 2.1f, 0.23f }
    };

    for (const auto& cloud : clouds)
    {
        const float cloudWidth = 170.0f * cloud.scale;
        const float cloudHeight = 68.0f * cloud.scale;
        const float span = width + cloudWidth + 240.0f;
        const float x = std::fmod (cloud.base * span + time * cloud.speed, span) - cloudWidth - 80.0f;
        drawCloud (g, x, cloud.y * height, cloudWidth, cloudHeight, cloud.alpha);
    }

    ColourGradient horizon (Colour (0x00FFF1D6), 0.0f, height * 0.72f,
                            Colour (0x55FFF0D2), 0.0f, height, false);
    g.setGradientFill (horizon);
    g.fillAll();
}

void SheetzEditor::drawCloud (Graphics& g, float x, float y, float width, float height, float alpha) const
{
    Path cloud;
    cloud.addEllipse (x, y + height * 0.27f, width * 0.52f, height * 0.64f);
    cloud.addEllipse (x + width * 0.18f, y + height * 0.04f, width * 0.44f, height * 0.76f);
    cloud.addEllipse (x + width * 0.43f, y + height * 0.16f, width * 0.48f, height * 0.68f);
    cloud.addRoundedRectangle (x + width * 0.07f, y + height * 0.50f,
                               width * 0.84f, height * 0.47f, height * 0.22f);

    Path shadow (cloud);
    shadow.applyTransform (AffineTransform::translation (0.0f, 4.0f));
    g.setColour (Colour (0x1014384F));
    g.fillPath (shadow);

    g.setColour (Colours::white.withAlpha (alpha));
    g.fillPath (cloud);

    g.setColour (Colours::white.withAlpha (alpha * 0.6f));
    g.fillEllipse (x + width * 0.28f, y + height * 0.13f, width * 0.26f, height * 0.22f);
}

void SheetzEditor::drawPanel (Graphics& g, Rectangle<int> bounds, float alpha) const
{
    if (bounds.isEmpty())
        return;

    auto rounded = bounds.toFloat().reduced (0.5f);
    g.setColour (Colours::white.withAlpha (alpha));
    g.fillRoundedRectangle (rounded, 18.0f);
    g.setColour (Colours::white.withAlpha (alpha + 0.22f));
    g.drawRoundedRectangle (rounded, 18.0f, 1.0f);
}

void SheetzEditor::resized()
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    auto full = getLocalBounds().reduced (14);
    auto header = full.removeFromTop (56);
    full.removeFromTop (8);
    auto footer = full.removeFromBottom (18);
    statusLabel.setBounds (footer);

    auto left = full.removeFromLeft (124);
    full.removeFromLeft (10);
    auto right = full.removeFromRight (224);
    full.removeFromLeft (10);
    auto center = full;

    globalPanelBounds = center.removeFromBottom (100);
    center.removeFromBottom (8);
    curvePanelBounds = center;
    rightPanelBounds = right;
    moduleRailBounds = left;

    const int headerY = header.getY();
    const int brandWidth = 250;
    const int presetX = header.getX() + brandWidth;

    // Lay the transport/preset buttons out from the right edge so the header
    // stays balanced at every supported window width.
    int edge = header.getRight();

    folderButton.setBounds (edge - 44, headerY + 12, 44, 30);
    edge -= 48;
    exportButton.setBounds (edge - 64, headerY + 12, 64, 30);
    edge -= 68;
    importButton.setBounds (edge - 64, headerY + 12, 64, 30);
    edge -= 68;
    saveButton.setBounds (edge - 58, headerY + 12, 58, 30);
    edge -= 66;
    nextButton.setBounds (edge - 30, headerY + 12, 30, 30);
    edge -= 34;
    prevButton.setBounds (edge - 30, headerY + 12, 30, 30);
    edge -= 34;

    const int presetWidth = jlimit (230, 420, edge - 6 - presetX);
    presetBox.setBounds (presetX, headerY + 10, presetWidth, 34);

    auto curveInner = curvePanelBounds.reduced (12);
    auto curveTop = curveInner.removeFromTop (34);
    auto curveControls = curveTop.removeFromRight (jmin (276, curveTop.getWidth()));
    curveTitleLabel.setBounds (curveTop);

    rateBox.setBounds (curveControls.removeFromRight (104).withSizeKeepingCentre (104, 28));
    curveControls.removeFromRight (8);
    snapButton.setBounds (curveControls.removeFromRight (52).withSizeKeepingCentre (52, 24));
    curveControls.removeFromRight (6);
    resetButton.setBounds (curveControls.removeFromRight (50).withSizeKeepingCentre (50, 24));
    curveControls.removeFromRight (6);
    flipButton.setBounds (curveControls.removeFromRight (50).withSizeKeepingCentre (50, 24));

    curveEditor.setBounds (curveInner.reduced (0, 2));

    auto rightInner = rightPanelBounds.reduced (12);
    moduleTitleLabel.setBounds (rightInner.removeFromTop (22));
    rightInner.removeFromTop (4);

    auto depthArea = rightInner.removeFromTop (112);
    depthKnob.label.setBounds (depthArea.removeFromTop (16));
    depthKnob.slider.setBounds (depthArea.withSizeKeepingCentre (98, 94));

    rightInner.removeFromTop (2);
    moduleHintLabel.setBounds (rightInner.removeFromTop (42));
    auto extra = rightInner.removeFromTop (148);

    auto placeKnob = [] (Knob& knob, Rectangle<int> bounds)
    {
        knob.label.setBounds (bounds.removeFromTop (14));
        knob.slider.setBounds (bounds.reduced (2));
    };

    auto filterArea = extra;
    auto filterTop = filterArea.removeFromTop (102);
    const int half = filterTop.getWidth() / 2;
    placeKnob (cutoffKnob, filterTop.removeFromLeft (half).reduced (2, 0));
    placeKnob (resoKnob, filterTop.reduced (2, 0));
    filterModeBox.setBounds (filterArea.removeFromTop (30).reduced (14, 0));

    placeKnob (toneKnob, extra.withSizeKeepingCentre (108, 124));

    auto crushArea = extra;
    const int crushHalf = crushArea.getWidth() / 2;
    placeKnob (bitsKnob, crushArea.removeFromLeft (crushHalf).reduced (2, 0));
    placeKnob (crushRateKnob, crushArea.reduced (2, 0));

    auto rightBottom = rightInner.removeFromBottom (74);
    onButton.setBounds (rightBottom.removeFromTop (30).withSizeKeepingCentre (152, 28));
    rightBottom.removeFromTop (4);
    presetInfoLabel.setBounds (rightBottom.removeFromTop (24));

    auto globalInner = globalPanelBounds.reduced (14);
    const int globalKnobWidth = 94;
    const int globalGap = 18;
    const int globalWidth = 4 * globalKnobWidth + 3 * globalGap;
    auto globalRow = globalInner.withSizeKeepingCentre (globalWidth, globalInner.getHeight());

    placeKnob (mixKnob, globalRow.removeFromLeft (globalKnobWidth));
    globalRow.removeFromLeft (globalGap);
    placeKnob (smoothKnob, globalRow.removeFromLeft (globalKnobWidth));
    globalRow.removeFromLeft (globalGap);
    placeKnob (outputKnob, globalRow.removeFromLeft (globalKnobWidth));
    globalRow.removeFromLeft (globalGap);
    placeKnob (phaseKnob, globalRow.removeFromLeft (globalKnobWidth));

    const int buttonTop = moduleRailBounds.getY() + 22;
    const int available = moduleRailBounds.getBottom() - buttonTop;
    const int buttonHeight = jlimit (50, 70, (available - 5 * 6) / 6);
    int buttonY = buttonTop;

    for (int m = 0; m < sheetz::NumModules; ++m)
    {
        moduleButtons[(size_t) m]->setBounds (moduleRailBounds.getX(), buttonY,
                                              moduleRailBounds.getWidth(), buttonHeight);
        buttonY += buttonHeight + 6;
    }
}

bool SheetzEditor::isInterestedInFileDrag (const StringArray& files)
{
    for (const auto& file : files)
        if (extensionIsPreset (file))
            return true;

    return false;
}

void SheetzEditor::filesDropped (const StringArray& files, int, int)
{
    int imported = 0;
    File lastFile;

    for (const auto& path : files)
    {
        const auto destination = processor.importPresetFile (File (path));

        if (destination != File())
        {
            ++imported;
            lastFile = destination;
        }
    }

    if (imported > 0)
    {
        lastPresetRevision = processor.getPresetRevision();
        refreshPresetList (lastFile);
        updateModuleUI();
        curveEditor.repaint();
        showStatus ("Imported  " + String (imported) + " preset(s)");
    }
    else
    {
        showStatus ("Drop a .sheetz preset file here");
    }
}
