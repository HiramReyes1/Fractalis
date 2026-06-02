/*
  ==============================================================================
    PluginEditor.h

    Declara todos los componentes visuales de la GUI y sus attachments.
    Cada oscilador tiene: enable toggle, wave combo, y knobs de VOL/ATK/DCY/SUS/REL.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class FractalisAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    FractalisAudioProcessorEditor (FractalisAudioProcessor&);
    ~FractalisAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    FractalisAudioProcessor& audioProcessor;

    // =========================================================================
    // CONTROLES DE OSC 1
    // =========================================================================

    // Titulo del panel y boton de activacion
    juce::Label        osc1Title;
    juce::ToggleButton osc1EnableButton;

    // Selector de forma de onda
    juce::Label    osc1WaveLabel;
    juce::ComboBox osc1WaveCombo;

    // Knobs: volumen y ADSR
    juce::Label  osc1VolumeLabel;   juce::Slider osc1VolumeSlider;
    juce::Label  osc1AttackLabel;   juce::Slider osc1AttackSlider;
    juce::Label  osc1DecayLabel;    juce::Slider osc1DecaySlider;
    juce::Label  osc1SustainLabel;  juce::Slider osc1SustainSlider;
    juce::Label  osc1ReleaseLabel;  juce::Slider osc1ReleaseSlider;

    // =========================================================================
    // CONTROLES DE OSC 2
    // =========================================================================

    juce::Label        osc2Title;
    juce::ToggleButton osc2EnableButton;

    juce::Label    osc2WaveLabel;
    juce::ComboBox osc2WaveCombo;

    juce::Label  osc2VolumeLabel;   juce::Slider osc2VolumeSlider;
    juce::Label  osc2AttackLabel;   juce::Slider osc2AttackSlider;
    juce::Label  osc2DecayLabel;    juce::Slider osc2DecaySlider;
    juce::Label  osc2SustainLabel;  juce::Slider osc2SustainSlider;
    juce::Label  osc2ReleaseLabel;  juce::Slider osc2ReleaseSlider;

    // =========================================================================
    // ATTACHMENTS (conexiones GUI <-> APVTS)
    //
    // Cada Attachment sincroniza automaticamente un control visual con su
    // parametro en el APVTS. Sin estos objetos, mover un knob no afectaria
    // el audio y la automatizacion del DAW no moveria los controles.
    //
    // Usamos unique_ptr porque los Attachments deben crearse despues de que
    // los controles existen, y deben destruirse antes que los controles.
    // =========================================================================

    // OSC 1 attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   osc1EnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc1VolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc1AttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc1DecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc1SustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc1ReleaseAttachment;

    // OSC 2 attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   osc2EnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc2VolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc2AttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc2DecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc2SustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   osc2ReleaseAttachment;

    // =========================================================================
    // HELPERS DE ESTILO
    //
    // Funciones privadas para aplicar el estilo Fractalis a los controles
    // sin repetir codigo en el constructor.
    // =========================================================================
    void setupKnob    (juce::Slider&   slider);
    void setupLabel   (juce::Label&    label, const juce::String& text);
    void setupCombo   (juce::ComboBox& combo);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractalisAudioProcessorEditor)
};