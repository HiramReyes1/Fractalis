/*
  ==============================================================================
    PluginEditor.cpp
    Aquí construimos, posicionamos y conectamos todos los controles visuales.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Paleta de colores Fractalis
namespace FractalisColors
{
    const juce::Colour background   { 0xFF0a0a0a };  // Negro profundo
    const juce::Colour surface      { 0xFF111111 };  // Superficie oscura
    const juce::Colour darkGreen    { 0xFF1a4a1a };  // Verde oscuro
    const juce::Colour midGreen     { 0xFF2d7a2d };  // Verde medio
    const juce::Colour accent       { 0xFF00c853 };  // Verde brillante (accent)
    const juce::Colour textPrimary  { 0xFFe0ffe0 };  // Verde blancuzco
    const juce::Colour textSecond   { 0xFF88bb88 };  // Verde apagado
}

//==============================================================================
// CONSTRUCTOR
// Aquí configuramos todos los controles y los conectamos al APVTS
//==============================================================================
FractalisAudioProcessorEditor::FractalisAudioProcessorEditor (FractalisAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // =========================================================================
    // CONFIGURAR EL COMBOBOX DE TIPO DE ONDA
    // =========================================================================

    // Etiqueta del ComboBox
    waveTypeLabel.setText ("OSCILLATOR", juce::dontSendNotification);
    waveTypeLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    waveTypeLabel.setColour (juce::Label::textColourId, FractalisColors::accent);
    waveTypeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (waveTypeLabel);

    // Agregar las opciones de onda al ComboBox
    // IMPORTANTE: los IDs empiezan en 1 (no en 0) en JUCE ComboBox
    waveTypeComboBox.addItem ("Sine",     1);
    waveTypeComboBox.addItem ("Saw",      2);
    waveTypeComboBox.addItem ("Square",   3);
    waveTypeComboBox.addItem ("Triangle", 4);

    // Colores del ComboBox
    waveTypeComboBox.setColour (juce::ComboBox::backgroundColourId,   FractalisColors::surface);
    waveTypeComboBox.setColour (juce::ComboBox::textColourId,         FractalisColors::textPrimary);
    waveTypeComboBox.setColour (juce::ComboBox::outlineColourId,      FractalisColors::darkGreen);
    waveTypeComboBox.setColour (juce::ComboBox::arrowColourId,        FractalisColors::accent);
    waveTypeComboBox.setColour (juce::ComboBox::focusedOutlineColourId, FractalisColors::accent);
    addAndMakeVisible (waveTypeComboBox);

    // Conectar el ComboBox al parámetro "waveType" del APVTS
    // A partir de aquí, el ComboBox y el parámetro están sincronizados
    waveTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (audioProcessor.apvts, "waveType", waveTypeComboBox);

    // =========================================================================
    // CONFIGURAR EL SLIDER DE VOLUMEN
    // =========================================================================

    volumeLabel.setText ("VOLUME", juce::dontSendNotification);
    volumeLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    volumeLabel.setColour (juce::Label::textColourId, FractalisColors::accent);
    volumeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (volumeLabel);

    // Estilo rotativo (como un knob físico)
    volumeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    volumeSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);

    // Colores del Slider
    volumeSlider.setColour (juce::Slider::rotarySliderFillColourId,   FractalisColors::accent);
    volumeSlider.setColour (juce::Slider::rotarySliderOutlineColourId, FractalisColors::darkGreen);
    volumeSlider.setColour (juce::Slider::thumbColourId,              FractalisColors::accent);
    volumeSlider.setColour (juce::Slider::textBoxTextColourId,        FractalisColors::textPrimary);
    volumeSlider.setColour (juce::Slider::textBoxBackgroundColourId,  FractalisColors::surface);
    volumeSlider.setColour (juce::Slider::textBoxOutlineColourId,     FractalisColors::darkGreen);
    addAndMakeVisible (volumeSlider);

    // Conectar el Slider al parámetro "volume" del APVTS
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.apvts, "volume", volumeSlider);

    // Tamaño de la ventana del plugin
    setSize (400, 300);
}

FractalisAudioProcessorEditor::~FractalisAudioProcessorEditor() {}

//==============================================================================
// PAINT — Dibuja el fondo y elementos gráficos estáticos
// Se llama automáticamente cuando la ventana necesita redibujarse
//==============================================================================
void FractalisAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fondo principal
    g.fillAll (FractalisColors::background);

    // Barra de título en la parte superior
    juce::Rectangle<int> titleBar (0, 0, getWidth(), 50);
    g.setColour (FractalisColors::darkGreen);
    g.fillRect (titleBar);

    // Línea de acento debajo del título
    g.setColour (FractalisColors::accent);
    g.fillRect (0, 48, getWidth(), 2);

    // Nombre del plugin
    g.setColour (FractalisColors::textPrimary);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawFittedText ("FRACTALIS", titleBar, juce::Justification::centred, 1);

    // Subtítulo
    /*g.setColour (FractalisColors::textSecond);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("SYNTHESIZER v0.1", 0, 32, getWidth(), 14,
                juce::Justification::centred);*/

    // Panel de fondo para los controles
    g.setColour (FractalisColors::surface);
    g.fillRoundedRectangle (10.0f, 60.0f, getWidth() - 20.0f, getHeight() - 70.0f, 8.0f);

    // Borde del panel
    g.setColour (FractalisColors::darkGreen);
    g.drawRoundedRectangle (10.0f, 60.0f, getWidth() - 20.0f, getHeight() - 70.0f, 8.0f, 1.0f);
}

//==============================================================================
// RESIZED — Posiciona todos los componentes cuando la ventana cambia de tamaño
// Se llama al inicio y cada vez que el usuario redimensiona la ventana
//==============================================================================
void FractalisAudioProcessorEditor::resized()
{
    // Área disponible para controles (debajo del título, con margen)
    auto area = getLocalBounds().reduced (20).withTrimmedTop (55);

    // Dividir el área en dos columnas
    auto leftColumn  = area.removeFromLeft (area.getWidth() / 2);
    auto rightColumn = area;

    // Columna izquierda: tipo de onda
    // La etiqueta ocupa 20px de alto, el control ocupa el resto
    waveTypeLabel.setBounds   (leftColumn.removeFromTop (20));
    waveTypeComboBox.setBounds (leftColumn.reduced (10).removeFromTop (30));

    // Columna derecha: volumen (knob rotativo)
    volumeLabel.setBounds  (rightColumn.removeFromTop (20));
    volumeSlider.setBounds (rightColumn.reduced (10));
}