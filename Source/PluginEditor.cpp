/*
  ==============================================================================
    PluginEditor.cpp

    Construye, estiliza, posiciona y conecta todos los controles de la GUI.
    La ventana tiene 820 x 560 pixeles: dos paneles de oscilador en la parte
    superior y un teclado de piano animado en la parte inferior.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// PALETA DE COLORES
//
// Definidos como constantes en un namespace para facil acceso y modificacion.
// El formato es 0xAARRGGBB (alpha, red, green, blue en hexadecimal).
//==============================================================================
namespace FractalisColors
{
    const juce::Colour background  { 0xFF0a0a0a };  // Negro profundo (fondo general)
    const juce::Colour surface     { 0xFF111111 };  // Superficie oscura (fondo de controles)
    const juce::Colour panel       { 0xFF141a14 };  // Verde muy oscuro (fondo de paneles)
    const juce::Colour darkGreen   { 0xFF1a4a1a };  // Verde oscuro (bordes, outlines)
    const juce::Colour accent      { 0xFF00c853 };  // Verde brillante (elementos activos)
    const juce::Colour separator   { 0xFF1f3f1f };  // Verde muy oscuro (divisores)
    const juce::Colour textPrimary { 0xFFe0ffe0 };  // Verde blancuzco (texto principal)
    const juce::Colour textSecond  { 0xFF88bb88 };  // Verde apagado (etiquetas secundarias)
}

//==============================================================================
// HELPER: aplica el estilo Fractalis a un knob rotativo
//==============================================================================
void FractalisAudioProcessorEditor::setupKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);

    // TextBoxBelow: la caja de texto con el valor numerico aparece debajo del knob.
    // false = no es de solo lectura (el usuario puede escribir el valor).
    // 60 x 18 = tamano de la caja de texto en pixeles.
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 18);

    slider.setColour (juce::Slider::rotarySliderFillColourId,    FractalisColors::accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, FractalisColors::darkGreen);
    slider.setColour (juce::Slider::thumbColourId,               FractalisColors::accent);
    slider.setColour (juce::Slider::textBoxTextColourId,         FractalisColors::textPrimary);
    slider.setColour (juce::Slider::textBoxBackgroundColourId,   FractalisColors::surface);
    slider.setColour (juce::Slider::textBoxOutlineColourId,      FractalisColors::separator);

    addAndMakeVisible (slider);
}

//==============================================================================
// HELPER: aplica el estilo Fractalis a una etiqueta de texto
//==============================================================================
void FractalisAudioProcessorEditor::setupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, FractalisColors::textSecond);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

//==============================================================================
// HELPER: aplica el estilo Fractalis a un ComboBox
//==============================================================================
void FractalisAudioProcessorEditor::setupCombo (juce::ComboBox& combo)
{
    combo.setColour (juce::ComboBox::backgroundColourId,      FractalisColors::surface);
    combo.setColour (juce::ComboBox::textColourId,            FractalisColors::textPrimary);
    combo.setColour (juce::ComboBox::outlineColourId,         FractalisColors::darkGreen);
    combo.setColour (juce::ComboBox::arrowColourId,           FractalisColors::accent);
    combo.setColour (juce::ComboBox::focusedOutlineColourId,  FractalisColors::accent);
    addAndMakeVisible (combo);
}

//==============================================================================
// CONSTRUCTOR
//
// Orden de operaciones para cada control:
//   1. Configurar propiedades visuales (texto, colores, estilo)
//   2. Llamar addAndMakeVisible() para que JUCE lo agregue y muestre
//   3. Crear el Attachment para conectarlo al parametro del APVTS
//
// Los Attachments deben crearse AL FINAL, despues de que el control
// ya existe y esta visible.
//==============================================================================
FractalisAudioProcessorEditor::FractalisAudioProcessorEditor (FractalisAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // =========================================================================
    // OSC 1
    // =========================================================================

    // Titulo del panel
    osc1Title.setText ("OSC 1", juce::dontSendNotification);
    osc1Title.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    osc1Title.setColour (juce::Label::textColourId, FractalisColors::accent);
    addAndMakeVisible (osc1Title);

    // Boton de activacion (ToggleButton = checkbox con texto)
    // tickColourId = color del tick/check cuando esta activo
    // tickDisabledColourId = color del indicador cuando esta inactivo
    osc1EnableButton.setButtonText ("ENABLE");
    osc1EnableButton.setColour (juce::ToggleButton::textColourId,         FractalisColors::textPrimary);
    osc1EnableButton.setColour (juce::ToggleButton::tickColourId,          FractalisColors::accent);
    osc1EnableButton.setColour (juce::ToggleButton::tickDisabledColourId,  FractalisColors::darkGreen);
    addAndMakeVisible (osc1EnableButton);
    osc1EnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "osc1Enabled", osc1EnableButton);

    // Selector de forma de onda
    // Nota: los IDs del ComboBox comienzan en 1, no en 0.
    // El Attachment traduce entre el indice del APVTS (0-3) y el ID del combo (1-4).
    setupLabel (osc1WaveLabel, "WAVEFORM");
    osc1WaveCombo.addItem ("Sine",     1);
    osc1WaveCombo.addItem ("Saw",      2);
    osc1WaveCombo.addItem ("Square",   3);
    osc1WaveCombo.addItem ("Triangle", 4);
    setupCombo (osc1WaveCombo);
    osc1WaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "osc1WaveType", osc1WaveCombo);

    // Knobs de volumen y ADSR
    setupLabel (osc1VolumeLabel,  "VOL");
    setupLabel (osc1AttackLabel,  "ATTACK");
    setupLabel (osc1DecayLabel,   "DECAY");
    setupLabel (osc1SustainLabel, "SUSTAIN");
    setupLabel (osc1ReleaseLabel, "RELEASE");

    setupKnob (osc1VolumeSlider);
    setupKnob (osc1AttackSlider);
    setupKnob (osc1DecaySlider);
    setupKnob (osc1SustainSlider);
    setupKnob (osc1ReleaseSlider);

    osc1VolumeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc1Volume",  osc1VolumeSlider);
    osc1AttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc1Attack",  osc1AttackSlider);
    osc1DecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc1Decay",   osc1DecaySlider);
    osc1SustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc1Sustain", osc1SustainSlider);
    osc1ReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc1Release", osc1ReleaseSlider);

    // =========================================================================
    // OSC 2 (estructura identica a OSC 1, parametros distintos)
    // =========================================================================

    osc2Title.setText ("OSC 2", juce::dontSendNotification);
    osc2Title.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    osc2Title.setColour (juce::Label::textColourId, FractalisColors::accent);
    addAndMakeVisible (osc2Title);

    osc2EnableButton.setButtonText ("ENABLE");
    osc2EnableButton.setColour (juce::ToggleButton::textColourId,         FractalisColors::textPrimary);
    osc2EnableButton.setColour (juce::ToggleButton::tickColourId,          FractalisColors::accent);
    osc2EnableButton.setColour (juce::ToggleButton::tickDisabledColourId,  FractalisColors::darkGreen);
    addAndMakeVisible (osc2EnableButton);
    osc2EnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "osc2Enabled", osc2EnableButton);

    setupLabel (osc2WaveLabel, "WAVEFORM");
    osc2WaveCombo.addItem ("Sine",     1);
    osc2WaveCombo.addItem ("Saw",      2);
    osc2WaveCombo.addItem ("Square",   3);
    osc2WaveCombo.addItem ("Triangle", 4);
    setupCombo (osc2WaveCombo);
    osc2WaveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.apvts, "osc2WaveType", osc2WaveCombo);

    setupLabel (osc2VolumeLabel,  "VOL");
    setupLabel (osc2AttackLabel,  "ATTACK");
    setupLabel (osc2DecayLabel,   "DECAY");
    setupLabel (osc2SustainLabel, "SUSTAIN");
    setupLabel (osc2ReleaseLabel, "RELEASE");

    setupKnob (osc2VolumeSlider);
    setupKnob (osc2AttackSlider);
    setupKnob (osc2DecaySlider);
    setupKnob (osc2SustainSlider);
    setupKnob (osc2ReleaseSlider);

    osc2VolumeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc2Volume",  osc2VolumeSlider);
    osc2AttackAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc2Attack",  osc2AttackSlider);
    osc2DecayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc2Decay",   osc2DecaySlider);
    osc2SustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc2Sustain", osc2SustainSlider);
    osc2ReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "osc2Release", osc2ReleaseSlider);

    // =========================================================================
    // TECLADO DE PIANO
    //
    // Se agrega al editor como componente hijo. El timer se encarga de
    // leer lastMidiNote del procesador y llamar setActiveNote() para
    // actualizar la visualizacion sin tocar el hilo de audio.
    // =========================================================================
    addAndMakeVisible (pianoDisplay);

    // Timer a ~33fps: suficiente para animacion fluida sin consumir CPU.
    // El intervalo de 30ms es mucho mayor que el buffer de audio (~3ms),
    // por lo que nunca interfiere con el procesamiento de audio.
    startTimerHz (33);

    // La ventana crece 120px en altura para alojar el teclado en la parte inferior
    setSize (820, 560);
}

FractalisAudioProcessorEditor::~FractalisAudioProcessorEditor()
{
    // Detener el timer antes de que se destruya el objeto para evitar
    // callbacks sobre memoria liberada
    stopTimer();
}

//==============================================================================
// PAINT
//
// Dibuja todos los elementos graficos estaticos (fondos, titulos, lineas).
// Se llama automaticamente cuando la ventana necesita redibujarse.
// Los controles JUCE (sliders, combos, etc.) se dibujan solos.
//==============================================================================
void FractalisAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Fondo principal
    g.fillAll (FractalisColors::background);

    // Barra de titulo
    g.setColour (FractalisColors::darkGreen);
    g.fillRect (0, 0, getWidth(), 50);

    // Linea de acento debajo del titulo
    g.setColour (FractalisColors::accent);
    g.fillRect (0, 48, getWidth(), 2);

    // Nombre del plugin
    g.setColour (FractalisColors::textPrimary);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawFittedText ("FRACTALIS", 0, 0, getWidth(), 36, juce::Justification::centred, 1);

    // Subtitulo
    g.setColour (FractalisColors::textSecond);
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("SYNTHESIZER v0.1", 0, 33, getWidth(), 13, juce::Justification::centred);

    const int halfW = getWidth() / 2;

    // Panel de fondo OSC 1
    g.setColour (FractalisColors::panel);
    g.fillRoundedRectangle (10.0f, 57.0f, (float)(halfW - 15), (float)(440 - 67), 6.0f);
    g.setColour (FractalisColors::separator);
    g.drawRoundedRectangle (10.0f, 57.0f, (float)(halfW - 15), (float)(440 - 67), 6.0f, 1.0f);

    // Panel de fondo OSC 2
    const float osc2X = (float)(halfW + 5);
    g.setColour (FractalisColors::panel);
    g.fillRoundedRectangle (osc2X, 57.0f, (float)(halfW - 15), (float)(440 - 67), 6.0f);
    g.setColour (FractalisColors::separator);
    g.drawRoundedRectangle (osc2X, 57.0f, (float)(halfW - 15), (float)(440 - 67), 6.0f, 1.0f);

    // -------------------------------------------------------------------------
    // SECCION DEL TECLADO DE PIANO
    //
    // Franja oscura de fondo para el area del piano, con separador superior.
    // El teclado en si lo dibuja PianoKeyboardDisplay::paint().
    // -------------------------------------------------------------------------
    const int pianoY = 445;

    // Fondo de la seccion del piano
    g.setColour (juce::Colour (0xFF0d140d));
    g.fillRect (0, pianoY - 5, getWidth(), getHeight() - pianoY + 5);

    // Linea separadora con acento
    g.setColour (FractalisColors::separator);
    g.fillRect (0, pianoY - 5, getWidth(), 1);
    g.setColour (FractalisColors::accent.withAlpha (0.35f));
    g.fillRect (0, pianoY - 4, getWidth(), 1);

    // Etiqueta "MIDI IN" a la izquierda del teclado
    g.setColour (FractalisColors::textSecond);
    g.setFont (juce::FontOptions (8.0f, juce::Font::bold));
    g.drawText ("MIDI IN", 8, pianoY - 3, 44, 10, juce::Justification::centredLeft, false);
}

//==============================================================================
// RESIZED
//
// Posiciona todos los controles dentro de sus paneles.
// Se llama al inicio y cada vez que el tamano de ventana cambia.
//
// Estrategia de layout: usamos juce::Rectangle<int> y sus metodos
// removeFromTop() / removeFromLeft() para "consumir" el espacio disponible
// de arriba hacia abajo y de izquierda a derecha.
//
// El mismo bloque de codigo se repite para OSC 1 y OSC 2, operando sobre
// diferentes areas (a1 y a2) y diferentes controles.
//==============================================================================
void FractalisAudioProcessorEditor::resized()
{
    const int panelMargin = 14;
    const int halfW       = getWidth() / 2;

    // Los paneles de osciladores ocupan los primeros 440px de altura,
    // igual que antes. Solo el setSize cambio de 440 a 560.
    const int oscPanelH = 440;

    // Area de contenido de cada panel (dentro del borde redondeado)
    auto a1 = juce::Rectangle<int> (10,        57, halfW - 15, oscPanelH - 67).reduced (panelMargin);
    auto a2 = juce::Rectangle<int> (halfW + 5, 57, halfW - 15, oscPanelH - 67).reduced (panelMargin);

    // =========================================================================
    // LAYOUT OSC 1
    // =========================================================================

    // Fila 1: titulo a la izquierda, boton enable a la derecha
    auto header1 = a1.removeFromTop (26);
    osc1Title.setBounds (header1.removeFromLeft (60));
    osc1EnableButton.setBounds (header1);
    a1.removeFromTop (8);

    // Fila 2: etiqueta y combo de forma de onda
    osc1WaveLabel.setBounds (a1.removeFromTop (16));
    osc1WaveCombo.setBounds (a1.removeFromTop (26));
    a1.removeFromTop (12);

    // Fila 3 + 4: etiquetas y knobs de VOL/ATK/DCY/SUS/REL
    // Cada knob ocupa 1/5 del ancho disponible
    const int knobW1 = a1.getWidth() / 5;

    auto labels1 = a1.removeFromTop (15);
    osc1VolumeLabel.setBounds  (labels1.removeFromLeft (knobW1));
    osc1AttackLabel.setBounds  (labels1.removeFromLeft (knobW1));
    osc1DecayLabel.setBounds   (labels1.removeFromLeft (knobW1));
    osc1SustainLabel.setBounds (labels1.removeFromLeft (knobW1));
    osc1ReleaseLabel.setBounds (labels1.removeFromLeft (knobW1));

    // Los knobs toman el espacio restante (con un maximo de 120px de alto)
    auto knobs1 = a1.removeFromTop (juce::jmin (a1.getHeight(), 120));
    osc1VolumeSlider.setBounds  (knobs1.removeFromLeft (knobW1));
    osc1AttackSlider.setBounds  (knobs1.removeFromLeft (knobW1));
    osc1DecaySlider.setBounds   (knobs1.removeFromLeft (knobW1));
    osc1SustainSlider.setBounds (knobs1.removeFromLeft (knobW1));
    osc1ReleaseSlider.setBounds (knobs1.removeFromLeft (knobW1));

    // =========================================================================
    // LAYOUT OSC 2 (identico a OSC 1, opera sobre a2)
    // =========================================================================

    auto header2 = a2.removeFromTop (26);
    osc2Title.setBounds (header2.removeFromLeft (60));
    osc2EnableButton.setBounds (header2);
    a2.removeFromTop (8);

    osc2WaveLabel.setBounds (a2.removeFromTop (16));
    osc2WaveCombo.setBounds (a2.removeFromTop (26));
    a2.removeFromTop (12);

    const int knobW2 = a2.getWidth() / 5;

    auto labels2 = a2.removeFromTop (15);
    osc2VolumeLabel.setBounds  (labels2.removeFromLeft (knobW2));
    osc2AttackLabel.setBounds  (labels2.removeFromLeft (knobW2));
    osc2DecayLabel.setBounds   (labels2.removeFromLeft (knobW2));
    osc2SustainLabel.setBounds (labels2.removeFromLeft (knobW2));
    osc2ReleaseLabel.setBounds (labels2.removeFromLeft (knobW2));

    auto knobs2 = a2.removeFromTop (juce::jmin (a2.getHeight(), 120));
    osc2VolumeSlider.setBounds  (knobs2.removeFromLeft (knobW2));
    osc2AttackSlider.setBounds  (knobs2.removeFromLeft (knobW2));
    osc2DecaySlider.setBounds   (knobs2.removeFromLeft (knobW2));
    osc2SustainSlider.setBounds (knobs2.removeFromLeft (knobW2));
    osc2ReleaseSlider.setBounds (knobs2.removeFromLeft (knobW2));

    // =========================================================================
    // LAYOUT DEL TECLADO DE PIANO
    //
    // Ocupa toda la anchura con margenes laterales de 10px.
    // Se posiciona en la franja inferior, dejando 8px de padding vertical.
    // La altura de 95px da espacio suficiente para teclas blancas y negras
    // con las etiquetas de octava visibles.
    // =========================================================================
    const int pianoMargin  = 10;
    const int pianoTop     = oscPanelH + 8;           // 448px desde arriba
    const int pianoHeight  = getHeight() - pianoTop - 8;  // ~104px

    pianoDisplay.setBounds (pianoMargin,
                            pianoTop,
                            getWidth() - pianoMargin * 2,
                            pianoHeight);
}