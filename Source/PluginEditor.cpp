/*
  ==============================================================================
    PluginEditor.cpp — Fractalis Synthesizer v0.2

    Ventana: 820 × 680 px
      Fila 1 (y 57–244)   : OSC 1 | OSC 2  — compacta (h=188)
      Fila 2 (y 251–569)  : LFO | MOD ENV | FILTER — alta (h=319)
      Piano  (y 578–672)  : teclado MIDI animado
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
// PALETA DE COLORES (igual que v0.1, definida en namespace para fácil acceso)
// =============================================================================
namespace FractalisColors
{
    const juce::Colour background  { 0xFF0a0a0a };
    const juce::Colour surface     { 0xFF111111 };
    const juce::Colour panel       { 0xFF141a14 };
    const juce::Colour darkGreen   { 0xFF1a4a1a };
    const juce::Colour accent      { 0xFF00c853 };
    const juce::Colour separator   { 0xFF1f3f1f };
    const juce::Colour textPrimary { 0xFFe0ffe0 };
    const juce::Colour textSecond  { 0xFF88bb88 };
    // Color de acento secundario para paneles de fila 2 (violeta/azul verdoso)
    const juce::Colour accent2     { 0xFF00b4d8 };
    const juce::Colour panel2      { 0xFF0d161d };
}

// =============================================================================
// HELPERS DE ESTILO
// =============================================================================

// Knob rotativo estilo Fractalis. Si bipolar=true, el punto de inicio
// está en el centro del arco (útil para Amount del ModEnv y similares).
void FractalisAudioProcessorEditor::setupKnob (juce::Slider& slider, bool bipolar)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);

    if (bipolar)
    {
        // El knob bipolar usa el color de acento secundario
        slider.setColour (juce::Slider::rotarySliderFillColourId,    FractalisColors::accent2);
        slider.setColour (juce::Slider::thumbColourId,               FractalisColors::accent2);
    }
    else
    {
        slider.setColour (juce::Slider::rotarySliderFillColourId,    FractalisColors::accent);
        slider.setColour (juce::Slider::thumbColourId,               FractalisColors::accent);
    }

    slider.setColour (juce::Slider::rotarySliderOutlineColourId, FractalisColors::darkGreen);
    slider.setColour (juce::Slider::textBoxTextColourId,         FractalisColors::textPrimary);
    slider.setColour (juce::Slider::textBoxBackgroundColourId,   FractalisColors::surface);
    slider.setColour (juce::Slider::textBoxOutlineColourId,      FractalisColors::separator);

    addAndMakeVisible (slider);
}

void FractalisAudioProcessorEditor::setupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, FractalisColors::textSecond);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void FractalisAudioProcessorEditor::setupCombo (juce::ComboBox& combo)
{
    combo.setColour (juce::ComboBox::backgroundColourId,     FractalisColors::surface);
    combo.setColour (juce::ComboBox::textColourId,           FractalisColors::textPrimary);
    combo.setColour (juce::ComboBox::outlineColourId,        FractalisColors::darkGreen);
    combo.setColour (juce::ComboBox::arrowColourId,          FractalisColors::accent);
    combo.setColour (juce::ComboBox::focusedOutlineColourId, FractalisColors::accent);
    addAndMakeVisible (combo);
}

// Título de panel con tamaño y color propios
void FractalisAudioProcessorEditor::setupTitle (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, FractalisColors::accent);
    addAndMakeVisible (label);
}

// Boton ENABLE para efectos FX — usa accent2 (azul-verdoso) para distinguirse
// visualmente de los controles verdes del sintetizador
void FractalisAudioProcessorEditor::setupEnableButton (juce::ToggleButton& btn,
                                                        const juce::String& text)
{
    btn.setButtonText (text);
    btn.setColour (juce::ToggleButton::textColourId,        FractalisColors::textPrimary);
    btn.setColour (juce::ToggleButton::tickColourId,         FractalisColors::accent2);
    btn.setColour (juce::ToggleButton::tickDisabledColourId, FractalisColors::darkGreen);
    addAndMakeVisible (btn);
}

// Titulo de panel FX — usa accent2 en lugar del accent verde del sintetizador
void FractalisAudioProcessorEditor::setupFxTitle (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, FractalisColors::accent2);
    addAndMakeVisible (label);
}

// =============================================================================
// UPDATE VIEW VISIBILITY
// Oculta/muestra los grupos de componentes segun la vista activa y actualiza
// los colores de las pestanas para indicar cual esta seleccionada.
// =============================================================================
void FractalisAudioProcessorEditor::updateViewVisibility()
{
    const bool isSynth = (currentView == EditorView::Synth);

    // Conmutar visibilidad de todos los componentes de cada vista
    for (auto* c : synthComponents) c->setVisible (isSynth);
    for (auto* c : fxComponents)    c->setVisible (!isSynth);

    // Pestana SYNTH: resaltada con accent verde cuando esta activa
    viewSynthBtn.setColour (juce::TextButton::buttonColourId,
        isSynth ? FractalisColors::accent   : FractalisColors::surface);
    viewSynthBtn.setColour (juce::TextButton::textColourOffId,
        isSynth ? FractalisColors::background : FractalisColors::textSecond);

    // Pestana FX: resaltada con accent2 azul-verdoso cuando esta activa
    viewFxBtn.setColour (juce::TextButton::buttonColourId,
        !isSynth ? FractalisColors::accent2   : FractalisColors::surface);
    viewFxBtn.setColour (juce::TextButton::textColourOffId,
        !isSynth ? FractalisColors::background : FractalisColors::textSecond);

    viewSynthBtn.repaint();
    viewFxBtn.repaint();

    // Actualizar layout y fondo solo despues de que el componente este dimensionado
    if (getWidth() > 0)
    {
        resized();
        repaint();
    }
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================
FractalisAudioProcessorEditor::FractalisAudioProcessorEditor (FractalisAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&fractalisLookAndFeel);
    // =========================================================================
    // OSC 1
    // =========================================================================
    setupTitle (osc1Title, "OSC 1");

    osc1EnableButton.setButtonText ("ENABLE");
    osc1EnableButton.setColour (juce::ToggleButton::textColourId,        FractalisColors::textPrimary);
    osc1EnableButton.setColour (juce::ToggleButton::tickColourId,         FractalisColors::accent);
    osc1EnableButton.setColour (juce::ToggleButton::tickDisabledColourId, FractalisColors::darkGreen);
    addAndMakeVisible (osc1EnableButton);
    osc1EnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "osc1Enabled", osc1EnableButton);

    setupLabel (osc1WaveLabel, "WAVEFORM");
    osc1WaveCombo.addItem ("Sine", 1); osc1WaveCombo.addItem ("Saw",      2);
    osc1WaveCombo.addItem ("Square", 3); osc1WaveCombo.addItem ("Triangle", 4);
    setupCombo (osc1WaveCombo);
    osc1WaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "osc1WaveType", osc1WaveCombo);

    setupLabel (osc1VolumeLabel,  "VOL");   setupKnob (osc1VolumeSlider);
    setupLabel (osc1AttackLabel,  "ATTACK"); setupKnob (osc1AttackSlider);
    setupLabel (osc1DecayLabel,   "DECAY");  setupKnob (osc1DecaySlider);
    setupLabel (osc1SustainLabel, "SUSTAIN"); setupKnob (osc1SustainSlider);
    setupLabel (osc1ReleaseLabel, "RELEASE"); setupKnob (osc1ReleaseSlider);

    osc1VolumeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc1Volume",  osc1VolumeSlider);
    osc1AttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc1Attack",  osc1AttackSlider);
    osc1DecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc1Decay",   osc1DecaySlider);
    osc1SustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc1Sustain", osc1SustainSlider);
    osc1ReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc1Release", osc1ReleaseSlider);

    // =========================================================================
    // OSC 2
    // =========================================================================
    setupTitle (osc2Title, "OSC 2");

    osc2EnableButton.setButtonText ("ENABLE");
    osc2EnableButton.setColour (juce::ToggleButton::textColourId,        FractalisColors::textPrimary);
    osc2EnableButton.setColour (juce::ToggleButton::tickColourId,         FractalisColors::accent);
    osc2EnableButton.setColour (juce::ToggleButton::tickDisabledColourId, FractalisColors::darkGreen);
    addAndMakeVisible (osc2EnableButton);
    osc2EnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "osc2Enabled", osc2EnableButton);

    setupLabel (osc2WaveLabel, "WAVEFORM");
    osc2WaveCombo.addItem ("Sine", 1); osc2WaveCombo.addItem ("Saw",      2);
    osc2WaveCombo.addItem ("Square", 3); osc2WaveCombo.addItem ("Triangle", 4);
    setupCombo (osc2WaveCombo);
    osc2WaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "osc2WaveType", osc2WaveCombo);

    setupLabel (osc2VolumeLabel,  "VOL");    setupKnob (osc2VolumeSlider);
    setupLabel (osc2AttackLabel,  "ATTACK"); setupKnob (osc2AttackSlider);
    setupLabel (osc2DecayLabel,   "DECAY");  setupKnob (osc2DecaySlider);
    setupLabel (osc2SustainLabel, "SUSTAIN"); setupKnob (osc2SustainSlider);
    setupLabel (osc2ReleaseLabel, "RELEASE"); setupKnob (osc2ReleaseSlider);

    osc2VolumeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc2Volume",  osc2VolumeSlider);
    osc2AttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc2Attack",  osc2AttackSlider);
    osc2DecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc2Decay",   osc2DecaySlider);
    osc2SustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc2Sustain", osc2SustainSlider);
    osc2ReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "osc2Release", osc2ReleaseSlider);

    // =========================================================================
    // LFO
    // =========================================================================
    setupTitle (lfoTitle, "LFO");

    setupLabel (lfoRateLabel,  "RATE");  setupKnob (lfoRateSlider);
    setupLabel (lfoDepthLabel, "DEPTH"); setupKnob (lfoDepthSlider);

    setupLabel (lfoWaveLabel, "WAVE");
    lfoWaveCombo.addItem ("Sine", 1); lfoWaveCombo.addItem ("Saw",      2);
    lfoWaveCombo.addItem ("Square", 3); lfoWaveCombo.addItem ("Triangle", 4);
    setupCombo (lfoWaveCombo);

    setupLabel (lfoDestLabel, "DEST");
    // Añadir destinos de modulación al combo (índices 1..N)
    for (int i = 0; i < kModDestNames.size(); ++i)
        lfoDestCombo.addItem (kModDestNames[i], i + 1);
    setupCombo (lfoDestCombo);

    lfoRateAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "lfoRate",  lfoRateSlider);
    lfoDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "lfoDepth", lfoDepthSlider);
    lfoWaveAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfoWave",  lfoWaveCombo);
    lfoDestAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "lfoDest",  lfoDestCombo);

    // =========================================================================
    // MOD ENV
    // =========================================================================
    setupTitle (modEnvTitle, "MOD ENV");

    setupLabel (modEnvAttackLabel,  "ATTACK");  setupKnob (modEnvAttackSlider);
    setupLabel (modEnvDecayLabel,   "DECAY");   setupKnob (modEnvDecaySlider);
    setupLabel (modEnvSustainLabel, "SUSTAIN"); setupKnob (modEnvSustainSlider);
    setupLabel (modEnvReleaseLabel, "RELEASE"); setupKnob (modEnvReleaseSlider);

    // Amount bipolar (−1 a +1) → knob con acento diferenciado
    setupLabel (modEnvAmountLabel, "AMOUNT");
    setupKnob (modEnvAmountSlider, /*bipolar=*/true);

    setupLabel (modEnvDestLabel, "DEST");
    for (int i = 0; i < kModDestNames.size(); ++i)
        modEnvDestCombo.addItem (kModDestNames[i], i + 1);
    setupCombo (modEnvDestCombo);

    modEnvAttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "modEnvAttack",  modEnvAttackSlider);
    modEnvDecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "modEnvDecay",   modEnvDecaySlider);
    modEnvSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "modEnvSustain", modEnvSustainSlider);
    modEnvReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "modEnvRelease", modEnvReleaseSlider);
    modEnvAmountAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "modEnvAmount",  modEnvAmountSlider);
    modEnvDestAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "modEnvDest",    modEnvDestCombo);

    // =========================================================================
    // FILTER
    // =========================================================================
    setupTitle (filterTitle, "FILTER");

    setupLabel (filterCutoffLabel, "CUTOFF"); setupKnob (filterCutoffSlider);
    setupLabel (filterResLabel,    "RES");    setupKnob (filterResSlider);

    setupLabel (filterModeLabel, "MODE");
    filterModeCombo.addItem ("LP",    1);
    filterModeCombo.addItem ("HP",    2);
    filterModeCombo.addItem ("BP",    3);
    filterModeCombo.addItem ("Notch", 4);
    setupCombo (filterModeCombo);

    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "filterCutoff", filterCutoffSlider);
    filterResAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>  (audioProcessor.apvts, "filterRes",    filterResSlider);
    filterModeAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "filterMode",   filterModeCombo);

    // =========================================================================
    // PIANO
    // =========================================================================
    addAndMakeVisible (pianoDisplay);

    pianoDisplay.onNoteEvent = [this](int note, bool isOn)
    {
        if (isOn)
            audioProcessor.addGuiMidiMessage (
                juce::MidiMessage::noteOn  (1, note, (juce::uint8) 100));
        else
            audioProcessor.addGuiMidiMessage (
                juce::MidiMessage::noteOff (1, note));
    };

    // =========================================================================
    // PESTANAS DE VISTA (header)
    // =========================================================================
    viewSynthBtn.setButtonText ("SYNTH");
    viewSynthBtn.onClick = [this]() { currentView = EditorView::Synth; updateViewVisibility(); };
    addAndMakeVisible (viewSynthBtn);

    viewFxBtn.setButtonText ("FX");
    viewFxBtn.onClick = [this]() { currentView = EditorView::Fx; updateViewVisibility(); };
    addAndMakeVisible (viewFxBtn);

    // Alias locales para reducir verbosidad al crear attachments FX
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    // =========================================================================
    // EQ
    // =========================================================================
    setupFxTitle      (fxEqTitle, "EQ");
    setupEnableButton (fxEqEnableBtn, "ENABLE");
    fxEqEnableAttachment = std::make_unique<BA> (audioProcessor.apvts, "fxEqEnabled", fxEqEnableBtn);

    // Los knobs de EQ son bipolares (-12 a +12 dB) — se pasa bipolar=true
    setupLabel (fxEqLowLabel,  "LOW");   setupKnob (fxEqLowSlider,  true);
    setupLabel (fxEqMidLabel,  "MID");   setupKnob (fxEqMidSlider,  true);
    setupLabel (fxEqHighLabel, "HIGH");  setupKnob (fxEqHighSlider, true);

    fxEqLowAttachment  = std::make_unique<SA> (audioProcessor.apvts, "fxEqLowGain",  fxEqLowSlider);
    fxEqMidAttachment  = std::make_unique<SA> (audioProcessor.apvts, "fxEqMidGain",  fxEqMidSlider);
    fxEqHighAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxEqHighGain", fxEqHighSlider);

    // =========================================================================
    // COMPRESOR
    // =========================================================================
    setupFxTitle      (fxCompTitle, "COMP");
    setupEnableButton (fxCompEnableBtn, "ENABLE");
    fxCompEnableAttachment = std::make_unique<BA> (audioProcessor.apvts, "fxCompEnabled", fxCompEnableBtn);

    setupLabel (fxCompThreshLabel, "THRESH");  setupKnob (fxCompThreshSlider);
    setupLabel (fxCompRatioLabel,  "RATIO");   setupKnob (fxCompRatioSlider);
    setupLabel (fxCompAttackLabel, "ATTACK");  setupKnob (fxCompAttackSlider);
    setupLabel (fxCompRelLabel,    "RELEASE"); setupKnob (fxCompRelSlider);
    setupLabel (fxCompMakeupLabel, "MAKEUP");  setupKnob (fxCompMakeupSlider);

    fxCompThreshAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxCompThresh",   fxCompThreshSlider);
    fxCompRatioAttachment  = std::make_unique<SA> (audioProcessor.apvts, "fxCompRatio",    fxCompRatioSlider);
    fxCompAttackAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxCompAttack",   fxCompAttackSlider);
    fxCompRelAttachment    = std::make_unique<SA> (audioProcessor.apvts, "fxCompRelease",  fxCompRelSlider);
    fxCompMakeupAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxCompMakeup",   fxCompMakeupSlider);

    // =========================================================================
    // SATURACION
    // =========================================================================
    setupFxTitle      (fxSatTitle, "SATURATE");
    setupEnableButton (fxSatEnableBtn, "ENABLE");
    fxSatEnableAttachment = std::make_unique<BA> (audioProcessor.apvts, "fxSatEnabled", fxSatEnableBtn);

    setupLabel (fxSatDriveLabel, "DRIVE"); setupKnob (fxSatDriveSlider);
    setupLabel (fxSatMixLabel,   "MIX");   setupKnob (fxSatMixSlider);

    fxSatDriveAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxSatDrive", fxSatDriveSlider);
    fxSatMixAttachment   = std::make_unique<SA> (audioProcessor.apvts, "fxSatMix",   fxSatMixSlider);

    // =========================================================================
    // DELAY
    // =========================================================================
    setupFxTitle      (fxDelayTitle, "DELAY");
    setupEnableButton (fxDelayEnableBtn, "ENABLE");
    fxDelayEnableAttachment = std::make_unique<BA> (audioProcessor.apvts, "fxDelayEnabled", fxDelayEnableBtn);

    setupLabel (fxDelayTimeLabel, "TIME");     setupKnob (fxDelayTimeSlider);
    setupLabel (fxDelayFbLabel,   "FEEDBACK"); setupKnob (fxDelayFbSlider);
    setupLabel (fxDelayMixLabel,  "MIX");      setupKnob (fxDelayMixSlider);

    fxDelayTimeAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxDelayTime",     fxDelayTimeSlider);
    fxDelayFbAttachment   = std::make_unique<SA> (audioProcessor.apvts, "fxDelayFeedback", fxDelayFbSlider);
    fxDelayMixAttachment  = std::make_unique<SA> (audioProcessor.apvts, "fxDelayMix",      fxDelayMixSlider);

    // =========================================================================
    // REVERB
    // =========================================================================
    setupFxTitle      (fxRevTitle, "REVERB");
    setupEnableButton (fxRevEnableBtn, "ENABLE");
    fxRevEnableAttachment = std::make_unique<BA> (audioProcessor.apvts, "fxRevEnabled", fxRevEnableBtn);

    setupLabel (fxRevSizeLabel,  "SIZE");  setupKnob (fxRevSizeSlider);
    setupLabel (fxRevDampLabel,  "DAMP");  setupKnob (fxRevDampSlider);
    setupLabel (fxRevWidthLabel, "WIDTH"); setupKnob (fxRevWidthSlider);
    setupLabel (fxRevMixLabel,   "MIX");   setupKnob (fxRevMixSlider);

    fxRevSizeAttachment  = std::make_unique<SA> (audioProcessor.apvts, "fxRevSize",    fxRevSizeSlider);
    fxRevDampAttachment  = std::make_unique<SA> (audioProcessor.apvts, "fxRevDamping", fxRevDampSlider);
    fxRevWidthAttachment = std::make_unique<SA> (audioProcessor.apvts, "fxRevWidth",   fxRevWidthSlider);
    fxRevMixAttachment   = std::make_unique<SA> (audioProcessor.apvts, "fxRevMix",     fxRevMixSlider);

    // =========================================================================
    // LISTAS DE COMPONENTES POR VISTA
    // Los punteros son no propietarios; los objetos son miembros del editor.
    // =========================================================================
    synthComponents = {
        &osc1Title,          &osc1EnableButton,    &osc1WaveLabel,      &osc1WaveCombo,
        &osc1VolumeLabel,    &osc1VolumeSlider,    &osc1AttackLabel,    &osc1AttackSlider,
        &osc1DecayLabel,     &osc1DecaySlider,     &osc1SustainLabel,   &osc1SustainSlider,
        &osc1ReleaseLabel,   &osc1ReleaseSlider,
        &osc2Title,          &osc2EnableButton,    &osc2WaveLabel,      &osc2WaveCombo,
        &osc2VolumeLabel,    &osc2VolumeSlider,    &osc2AttackLabel,    &osc2AttackSlider,
        &osc2DecayLabel,     &osc2DecaySlider,     &osc2SustainLabel,   &osc2SustainSlider,
        &osc2ReleaseLabel,   &osc2ReleaseSlider,
        &lfoTitle,           &lfoRateLabel,        &lfoRateSlider,      &lfoDepthLabel,
        &lfoDepthSlider,     &lfoWaveLabel,        &lfoWaveCombo,       &lfoDestLabel,
        &lfoDestCombo,
        &modEnvTitle,        &modEnvAttackLabel,   &modEnvAttackSlider,
        &modEnvDecayLabel,   &modEnvDecaySlider,   &modEnvSustainLabel, &modEnvSustainSlider,
        &modEnvReleaseLabel, &modEnvReleaseSlider, &modEnvAmountLabel,  &modEnvAmountSlider,
        &modEnvDestLabel,    &modEnvDestCombo,
        &filterTitle,        &filterCutoffLabel,   &filterCutoffSlider,
        &filterResLabel,     &filterResSlider,     &filterModeLabel,    &filterModeCombo
    };

    fxComponents = {
        &fxEqTitle,          &fxEqEnableBtn,
        &fxEqLowLabel,       &fxEqLowSlider,      &fxEqMidLabel,       &fxEqMidSlider,
        &fxEqHighLabel,      &fxEqHighSlider,
        &fxCompTitle,        &fxCompEnableBtn,
        &fxCompThreshLabel,  &fxCompThreshSlider, &fxCompRatioLabel,   &fxCompRatioSlider,
        &fxCompAttackLabel,  &fxCompAttackSlider, &fxCompRelLabel,     &fxCompRelSlider,
        &fxCompMakeupLabel,  &fxCompMakeupSlider,
        &fxSatTitle,         &fxSatEnableBtn,
        &fxSatDriveLabel,    &fxSatDriveSlider,   &fxSatMixLabel,      &fxSatMixSlider,
        &fxDelayTitle,       &fxDelayEnableBtn,
        &fxDelayTimeLabel,   &fxDelayTimeSlider,  &fxDelayFbLabel,     &fxDelayFbSlider,
        &fxDelayMixLabel,    &fxDelayMixSlider,
        &fxRevTitle,         &fxRevEnableBtn,
        &fxRevSizeLabel,     &fxRevSizeSlider,    &fxRevDampLabel,     &fxRevDampSlider,
        &fxRevWidthLabel,    &fxRevWidthSlider,   &fxRevMixLabel,      &fxRevMixSlider
    };

    // Ocultar FX desde el inicio (vista Synth activa por defecto)
    for (auto* c : fxComponents) c->setVisible (false);

    startTimerHz (33);  // ~30ms: suficiente para animación fluida
    setSize (820, 680);
}

FractalisAudioProcessorEditor::~FractalisAudioProcessorEditor()
{
    setLookAndFeel(nullptr); 
    stopTimer();
}

// =============================================================================
// PAINT — fondos, títulos y líneas decorativas (los controles se pintan solos)
// =============================================================================
void FractalisAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (FractalisColors::background);

    // Barra de título
    g.setColour (FractalisColors::darkGreen);
    g.fillRect (0, 0, getWidth(), 50);
    g.setColour (FractalisColors::accent);
    g.fillRect (0, 48, getWidth(), 2);

    g.setColour (FractalisColors::textPrimary);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawFittedText ("FRACTALIS", 0, 0, getWidth(), 36, juce::Justification::centred, 1);


    // -------------------------------------------------------------------------
    // Subtitulo dinamico segun la vista activa
    // -------------------------------------------------------------------------
    g.setColour (FractalisColors::textSecond);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText (currentView == EditorView::Fx ? "FX CHAIN" : "SYNTHESIZER",
                0, 33, getWidth(), 13, juce::Justification::centred);

    const int halfW  = getWidth() / 2;
    const int thirdW = getWidth() / 3;

    if (currentView == EditorView::Synth)
    {
        // -------------------------------------------------------------------------
        // FILA 1 — Paneles OSC 1 y OSC 2 (y 57–244, h=188 compacto)
        // -------------------------------------------------------------------------
        const float oscPanelH = 188.0f;

        g.setColour (FractalisColors::panel);
        g.fillRoundedRectangle (10.0f, 57.0f, (float)(halfW - 15), oscPanelH, 6.0f);
        g.setColour (FractalisColors::separator);
        g.drawRoundedRectangle (10.0f, 57.0f, (float)(halfW - 15), oscPanelH, 6.0f, 1.0f);

        g.setColour (FractalisColors::panel);
        g.fillRoundedRectangle ((float)(halfW + 5), 57.0f, (float)(halfW - 15), oscPanelH, 6.0f);
        g.setColour (FractalisColors::separator);
        g.drawRoundedRectangle ((float)(halfW + 5), 57.0f, (float)(halfW - 15), oscPanelH, 6.0f, 1.0f);

        // -------------------------------------------------------------------------
        // FILA 2 — Paneles LFO | MOD ENV | FILTER (y 251–569, h=319 amplia)
        // -------------------------------------------------------------------------
        const float row2Y = 251.0f;
        const float row2H = 319.0f;

        g.setColour (FractalisColors::separator);
        g.fillRect (0, 245, getWidth(), 1);
        g.setColour (FractalisColors::accent.withAlpha (0.25f));
        g.fillRect (0, 246, getWidth(), 1);

        for (int col = 0; col < 3; ++col)
        {
            const float px = 10.0f + col * (float)thirdW;
            const float pw = (float)thirdW - 10.0f;
            g.setColour (FractalisColors::panel2);
            g.fillRoundedRectangle (px, row2Y, pw, row2H, 6.0f);
            g.setColour (FractalisColors::darkGreen);
            g.drawRoundedRectangle (px, row2Y, pw, row2H, 6.0f, 1.0f);
        }
    }
    else // EditorView::Fx
    {
        // -------------------------------------------------------------------------
        // FILA FX 1 — EQ | Compresor | Saturacion (y 57, h=242)
        // -------------------------------------------------------------------------
        const float fxRow1Y = 57.0f;
        const float fxRow1H = 242.0f;

        for (int col = 0; col < 3; ++col)
        {
            const float px = 10.0f + col * (float)thirdW;
            const float pw = (float)thirdW - 10.0f;
            g.setColour (FractalisColors::panel2);
            g.fillRoundedRectangle (px, fxRow1Y, pw, fxRow1H, 6.0f);
            g.setColour (FractalisColors::darkGreen);
            g.drawRoundedRectangle (px, fxRow1Y, pw, fxRow1H, 6.0f, 1.0f);
        }

        // Separador entre filas FX
        g.setColour (FractalisColors::separator);
        g.fillRect (0, (int)(fxRow1Y + fxRow1H) + 3, getWidth(), 1);
        g.setColour (FractalisColors::accent2.withAlpha (0.25f));
        g.fillRect (0, (int)(fxRow1Y + fxRow1H) + 4, getWidth(), 1);

        // -------------------------------------------------------------------------
        // FILA FX 2 — Delay | Reverb (y 311, h=260)
        // -------------------------------------------------------------------------
        const float fxRow2Y = 311.0f;
        const float fxRow2H = 260.0f;

        g.setColour (FractalisColors::panel2);
        g.fillRoundedRectangle (10.0f, fxRow2Y, (float)(halfW - 15), fxRow2H, 6.0f);
        g.setColour (FractalisColors::darkGreen);
        g.drawRoundedRectangle (10.0f, fxRow2Y, (float)(halfW - 15), fxRow2H, 6.0f, 1.0f);

        g.setColour (FractalisColors::panel2);
        g.fillRoundedRectangle ((float)(halfW + 5), fxRow2Y, (float)(halfW - 15), fxRow2H, 6.0f);
        g.setColour (FractalisColors::darkGreen);
        g.drawRoundedRectangle ((float)(halfW + 5), fxRow2Y, (float)(halfW - 15), fxRow2H, 6.0f, 1.0f);
    }

    // -------------------------------------------------------------------------
    // PIANO — franja inferior (y 578 – 672) — igual en ambas vistas
    // -------------------------------------------------------------------------
    const int pianoY = 578;
    g.setColour (juce::Colour (0xFF0d140d));
    g.fillRect (0, pianoY - 5, getWidth(), getHeight() - pianoY + 5);
    g.setColour (FractalisColors::separator);
    g.fillRect (0, pianoY - 5, getWidth(), 1);
    g.setColour (FractalisColors::accent.withAlpha (0.35f));
    g.fillRect (0, pianoY - 4, getWidth(), 1);
    g.setColour (FractalisColors::textSecond);
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText ("MIDI IN", 8, pianoY - 3, 44, 10, juce::Justification::centredLeft, false);
}

// =============================================================================
// RESIZED — posiciona todos los controles usando removeFromTop/Left
// =============================================================================
void FractalisAudioProcessorEditor::resized()
{
    const int panelMargin = 12;
    const int halfW  = getWidth() / 2;
    const int thirdW = getWidth() / 3;

    // Pestanas de vista — siempre posicionadas en el header, independiente de la vista
    viewSynthBtn.setBounds (10, 13, 62, 24);
    viewFxBtn.setBounds    (78, 13, 40, 24);

    if (currentView == EditorView::Synth)
    {
        // =========================================================================
        // FILA 1 — OSC 1 y OSC 2 (y 57, h=188 compacto)
        // =========================================================================
        auto a1 = juce::Rectangle<int> (10,        57, halfW - 15, 188).reduced (panelMargin);
        auto a2 = juce::Rectangle<int> (halfW + 5, 57, halfW - 15, 188).reduced (panelMargin);

        auto layoutOscPanel = [&](juce::Rectangle<int>& a,
                                   juce::Label& title, juce::ToggleButton& enableBtn,
                                   juce::Label& waveLabel, juce::ComboBox& waveCombo,
                                   juce::Label& volLbl,  juce::Slider& volSlider,
                                   juce::Label& atkLbl,  juce::Slider& atkSlider,
                                   juce::Label& dcyLbl,  juce::Slider& dcySlider,
                                   juce::Label& susLbl,  juce::Slider& susSlider,
                                   juce::Label& relLbl,  juce::Slider& relSlider)
        {
            auto header = a.removeFromTop (22);
            title.setBounds (header.removeFromLeft (60));
            enableBtn.setBounds (header);
            a.removeFromTop (3);
            waveLabel.setBounds (a.removeFromTop (13));
            waveCombo.setBounds (a.removeFromTop (22));
            a.removeFromTop (5);
            const int kw = a.getWidth() / 5;
            auto lblRow = a.removeFromTop (13);
            volLbl.setBounds (lblRow.removeFromLeft (kw)); atkLbl.setBounds (lblRow.removeFromLeft (kw));
            dcyLbl.setBounds (lblRow.removeFromLeft (kw)); susLbl.setBounds (lblRow.removeFromLeft (kw));
            relLbl.setBounds (lblRow.removeFromLeft (kw));
            auto kRow = a.removeFromTop (82);
            volSlider.setBounds (kRow.removeFromLeft (kw)); atkSlider.setBounds (kRow.removeFromLeft (kw));
            dcySlider.setBounds (kRow.removeFromLeft (kw)); susSlider.setBounds (kRow.removeFromLeft (kw));
            relSlider.setBounds (kRow.removeFromLeft (kw));
        };

        layoutOscPanel (a1,
            osc1Title, osc1EnableButton, osc1WaveLabel, osc1WaveCombo,
            osc1VolumeLabel, osc1VolumeSlider, osc1AttackLabel,  osc1AttackSlider,
            osc1DecayLabel,  osc1DecaySlider,  osc1SustainLabel, osc1SustainSlider,
            osc1ReleaseLabel, osc1ReleaseSlider);

        layoutOscPanel (a2,
            osc2Title, osc2EnableButton, osc2WaveLabel, osc2WaveCombo,
            osc2VolumeLabel, osc2VolumeSlider, osc2AttackLabel,  osc2AttackSlider,
            osc2DecayLabel,  osc2DecaySlider,  osc2SustainLabel, osc2SustainSlider,
            osc2ReleaseLabel, osc2ReleaseSlider);

        // =========================================================================
        // FILA 2 — LFO | MOD ENV | FILTER (y 251, h=319 amplia)
        // =========================================================================
        const int row2Y = 251;
        const int row2H = 319;

        auto lfoArea    = juce::Rectangle<int> (10,          row2Y, thirdW - 10, row2H).reduced (panelMargin);
        auto modEnvArea = juce::Rectangle<int> (thirdW,      row2Y, thirdW - 10, row2H).reduced (panelMargin);
        auto filterArea = juce::Rectangle<int> (thirdW * 2,  row2Y, thirdW - 10, row2H).reduced (panelMargin);

        // LFO — titulo, Rate + Depth (knobs 120px), Wave + Dest (combos)
        {
            auto& a = lfoArea;
            lfoTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (10);
            const int kw = a.getWidth() / 2;
            auto lblRow = a.removeFromTop (13);
            lfoRateLabel.setBounds  (lblRow.removeFromLeft (kw));
            lfoDepthLabel.setBounds (lblRow);
            auto kRow = a.removeFromTop (120);
            lfoRateSlider.setBounds  (kRow.removeFromLeft (kw));
            lfoDepthSlider.setBounds (kRow);
            a.removeFromTop (12);
            lfoWaveLabel.setBounds (a.removeFromTop (13));
            lfoWaveCombo.setBounds (a.removeFromTop (24));
            a.removeFromTop (8);
            lfoDestLabel.setBounds (a.removeFromTop (13));
            lfoDestCombo.setBounds (a.removeFromTop (24));
        }

        // MOD ENV — titulo, fila1: ATK/DCY/SUS/REL, fila2: AMOUNT + DEST
        {
            auto& a = modEnvArea;
            modEnvTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (8);
            const int kw4 = a.getWidth() / 4;
            auto lblRow1 = a.removeFromTop (13);
            modEnvAttackLabel.setBounds  (lblRow1.removeFromLeft (kw4));
            modEnvDecayLabel.setBounds   (lblRow1.removeFromLeft (kw4));
            modEnvSustainLabel.setBounds (lblRow1.removeFromLeft (kw4));
            modEnvReleaseLabel.setBounds (lblRow1);
            auto kRow1 = a.removeFromTop (85);
            modEnvAttackSlider.setBounds  (kRow1.removeFromLeft (kw4));
            modEnvDecaySlider.setBounds   (kRow1.removeFromLeft (kw4));
            modEnvSustainSlider.setBounds (kRow1.removeFromLeft (kw4));
            modEnvReleaseSlider.setBounds (kRow1);
            a.removeFromTop (6);
            const int amountW = a.getWidth() / 2;
            auto lblRow2 = a.removeFromTop (13);
            modEnvAmountLabel.setBounds (lblRow2.removeFromLeft (amountW));
            modEnvDestLabel.setBounds   (lblRow2);
            auto splitRow = a.removeFromTop (85);
            modEnvAmountSlider.setBounds (splitRow.removeFromLeft (amountW));
            modEnvDestCombo.setBounds    (splitRow.withSizeKeepingCentre (splitRow.getWidth(), 24));
        }

        // FILTER — titulo, Cutoff + Res (knobs 120px), Mode (combo)
        {
            auto& a = filterArea;
            filterTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (10);
            const int kw = a.getWidth() / 2;
            auto lblRow = a.removeFromTop (13);
            filterCutoffLabel.setBounds (lblRow.removeFromLeft (kw));
            filterResLabel.setBounds    (lblRow);
            auto kRow = a.removeFromTop (120);
            filterCutoffSlider.setBounds (kRow.removeFromLeft (kw));
            filterResSlider.setBounds    (kRow);
            a.removeFromTop (20);
            filterModeLabel.setBounds (a.removeFromTop (13));
            filterModeCombo.setBounds (a.removeFromTop (24));
        }
    }
    else // EditorView::Fx
    {
        // =========================================================================
        // FILA FX 1 — EQ | Compresor | Saturacion (y 57, h=242)
        // =========================================================================
        const int fxRow1Y = 57;
        const int fxRow1H = 242;

        // EQ — 3 knobs bipolares en una fila
        {
            auto a = juce::Rectangle<int> (10, fxRow1Y, thirdW - 10, fxRow1H).reduced (panelMargin);
            fxEqTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (8);
            fxEqEnableBtn.setBounds (a.removeFromTop (22));
            a.removeFromTop (5);
            const int kw = a.getWidth() / 3;
            auto lblRow = a.removeFromTop (13);
            fxEqLowLabel.setBounds  (lblRow.removeFromLeft (kw));
            fxEqMidLabel.setBounds  (lblRow.removeFromLeft (kw));
            fxEqHighLabel.setBounds (lblRow);
            auto kRow = a.removeFromTop (120);
            fxEqLowSlider.setBounds  (kRow.removeFromLeft (kw));
            fxEqMidSlider.setBounds  (kRow.removeFromLeft (kw));
            fxEqHighSlider.setBounds (kRow);
        }

        // Compresor — 5 knobs en una fila
        {
            auto a = juce::Rectangle<int> (thirdW, fxRow1Y, thirdW - 10, fxRow1H).reduced (panelMargin);
            fxCompTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (8);
            fxCompEnableBtn.setBounds (a.removeFromTop (22));
            a.removeFromTop (5);
            const int kw = a.getWidth() / 5;
            auto lblRow = a.removeFromTop (13);
            fxCompThreshLabel.setBounds (lblRow.removeFromLeft (kw));
            fxCompRatioLabel.setBounds  (lblRow.removeFromLeft (kw));
            fxCompAttackLabel.setBounds (lblRow.removeFromLeft (kw));
            fxCompRelLabel.setBounds    (lblRow.removeFromLeft (kw));
            fxCompMakeupLabel.setBounds (lblRow);
            auto kRow = a.removeFromTop (100);
            fxCompThreshSlider.setBounds (kRow.removeFromLeft (kw));
            fxCompRatioSlider.setBounds  (kRow.removeFromLeft (kw));
            fxCompAttackSlider.setBounds (kRow.removeFromLeft (kw));
            fxCompRelSlider.setBounds    (kRow.removeFromLeft (kw));
            fxCompMakeupSlider.setBounds (kRow);
        }

        // Saturacion — 2 knobs
        {
            auto a = juce::Rectangle<int> (thirdW * 2, fxRow1Y, thirdW - 10, fxRow1H).reduced (panelMargin);
            fxSatTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (8);
            fxSatEnableBtn.setBounds (a.removeFromTop (22));
            a.removeFromTop (5);
            const int kw = a.getWidth() / 2;
            auto lblRow = a.removeFromTop (13);
            fxSatDriveLabel.setBounds (lblRow.removeFromLeft (kw));
            fxSatMixLabel.setBounds   (lblRow);
            auto kRow = a.removeFromTop (120);
            fxSatDriveSlider.setBounds (kRow.removeFromLeft (kw));
            fxSatMixSlider.setBounds   (kRow);
        }

        // =========================================================================
        // FILA FX 2 — Delay | Reverb (y 311, h=260)
        // =========================================================================
        const int fxRow2Y = 311;
        const int fxRow2H = 260;

        // Delay — 3 knobs
        {
            auto a = juce::Rectangle<int> (10, fxRow2Y, halfW - 15, fxRow2H).reduced (panelMargin);
            fxDelayTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (10);
            fxDelayEnableBtn.setBounds (a.removeFromTop (22));
            a.removeFromTop (5);
            const int kw = a.getWidth() / 3;
            auto lblRow = a.removeFromTop (13);
            fxDelayTimeLabel.setBounds (lblRow.removeFromLeft (kw));
            fxDelayFbLabel.setBounds   (lblRow.removeFromLeft (kw));
            fxDelayMixLabel.setBounds  (lblRow);
            auto kRow = a.removeFromTop (130);
            fxDelayTimeSlider.setBounds (kRow.removeFromLeft (kw));
            fxDelayFbSlider.setBounds   (kRow.removeFromLeft (kw));
            fxDelayMixSlider.setBounds  (kRow);
        }

        // Reverb — 4 knobs
        {
            auto a = juce::Rectangle<int> (halfW + 5, fxRow2Y, halfW - 15, fxRow2H).reduced (panelMargin);
            fxRevTitle.setBounds (a.removeFromTop (18));
            a.removeFromTop (10);
            fxRevEnableBtn.setBounds (a.removeFromTop (22));
            a.removeFromTop (5);
            const int kw = a.getWidth() / 4;
            auto lblRow = a.removeFromTop (13);
            fxRevSizeLabel.setBounds  (lblRow.removeFromLeft (kw));
            fxRevDampLabel.setBounds  (lblRow.removeFromLeft (kw));
            fxRevWidthLabel.setBounds (lblRow.removeFromLeft (kw));
            fxRevMixLabel.setBounds   (lblRow);
            auto kRow = a.removeFromTop (130);
            fxRevSizeSlider.setBounds  (kRow.removeFromLeft (kw));
            fxRevDampSlider.setBounds  (kRow.removeFromLeft (kw));
            fxRevWidthSlider.setBounds (kRow.removeFromLeft (kw));
            fxRevMixSlider.setBounds   (kRow);
        }
    }

    // =========================================================================
    // PIANO — franja inferior (y 578, altura dinamica) — igual en ambas vistas
    // =========================================================================
    const int pianoMargin = 10;
    const int pianoTop    = 578;
    const int pianoHeight = getHeight() - pianoTop - 8;
    pianoDisplay.setBounds (pianoMargin, pianoTop,
                            getWidth() - pianoMargin * 2, pianoHeight);
}