/*
  ==============================================================================
    PluginEditor.h
    Declaramos todos los componentes visuales de la GUI.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class FractalisAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    FractalisAudioProcessorEditor (FractalisAudioProcessor&);
    ~FractalisAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Referencia al procesador para acceder al APVTS
    FractalisAudioProcessor& audioProcessor;

    // =========================================================================
    // COMPONENTES DE LA GUI
    // =========================================================================

    // ComboBox: menú desplegable para elegir el tipo de onda
    juce::ComboBox waveTypeComboBox;
    juce::Label    waveTypeLabel;

    // Slider: control deslizante para el volumen
    juce::Slider volumeSlider;
    juce::Label  volumeLabel;

    // =========================================================================
    // ATTACHMENTS (CONEXIONES GUI ↔ PARÁMETROS)
    // Los Attachments sincronizan automáticamente el control visual
    // con el parámetro del APVTS. Si el DAW automatiza el parámetro,
    // el control se mueve solo. Si el usuario mueve el control,
    // el parámetro se actualiza solo.
    // =========================================================================
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   volumeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractalisAudioProcessorEditor)
};