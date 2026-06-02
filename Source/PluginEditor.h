/*
  ==============================================================================
    PluginEditor.h

    Declara todos los componentes visuales de la GUI y sus attachments.
    Cada oscilador tiene: enable toggle, wave combo, y knobs de VOL/ATK/DCY/SUS/REL.
    La parte inferior de la ventana contiene un teclado de piano animado que
    responde a los eventos MIDI en tiempo real.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// PIANO KEYBOARD DISPLAY
//
// Componente autonomo que dibuja un teclado de piano de dos octavas (C3-B4,
// notas 48-71) y resalta la tecla activa segun el valor de lastMidiNote del
// procesador. Hereda de juce::Component para poder recibir setBounds() y
// participar en el layout del editor.
//
// Diseño:
//   - Teclas blancas: rectangulos con borde verde oscuro
//   - Teclas negras: rectangulos mas cortos superpuestos encima
//   - Tecla activa: relleno con el color accent (verde brillante) + glow
//   - Etiqueta de octava debajo de cada C
//==============================================================================
class PianoKeyboardDisplay : public juce::Component
{
public:
    // firstNote / lastNote definen el rango visible del teclado.
    // Por defecto mostramos C2 a B4 (3 octavas, notas 36-71 = 21 teclas blancas).
    PianoKeyboardDisplay (int firstNote = 36, int lastNote = 71)
        : rangeStart (firstNote), rangeEnd (lastNote)
    {
    }

    // activeNote: la nota MIDI que debe iluminarse. -1 = ninguna.
    void setActiveNote (int midiNote)
    {
        if (activeNote != midiNote)
        {
            activeNote = midiNote;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        // -----------------------------------------------------------------
        // Geometria: calculamos cuantas teclas blancas hay en el rango y
        // dividimos el ancho disponible entre ellas.
        // -----------------------------------------------------------------
        const int numWhite = countWhiteKeys (rangeStart, rangeEnd);
        if (numWhite == 0) return;

        const float totalW  = (float) getWidth();
        const float totalH  = (float) getHeight();
        const float whiteW  = totalW / (float) numWhite;
        const float whiteH  = totalH;
        const float blackW  = whiteW * 0.58f;
        const float blackH  = whiteH * 0.60f;

        // -----------------------------------------------------------------
        // PASO 1: dibujar todas las teclas blancas
        // -----------------------------------------------------------------
        int whiteIdx = 0;
        for (int note = rangeStart; note <= rangeEnd; ++note)
        {
            if (isBlackKey (note)) continue;

            const float x = whiteIdx * whiteW;
            juce::Rectangle<float> keyRect (x + 1.0f, 0.0f, whiteW - 2.0f, whiteH - 1.0f);

            const bool isActive = (note == activeNote);

            if (isActive)
            {
                // Tecla activa: gradiente verde brillante de arriba a abajo
                juce::ColourGradient grad (
                    juce::Colour (0xFF00ff66), x + whiteW * 0.5f, 0.0f,
                    juce::Colour (0xFF004422), x + whiteW * 0.5f, whiteH,
                    false);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (keyRect, 3.0f);

                // Glow exterior
                g.setColour (juce::Colour (0x6600c853));
                g.drawRoundedRectangle (keyRect.expanded (1.5f), 3.0f, 2.0f);
            }
            else
            {
                // Tecla normal: gradiente sutil de blanco a gris claro
                juce::ColourGradient grad (
                    juce::Colour (0xFFd8eed8), x + whiteW * 0.5f, 0.0f,
                    juce::Colour (0xFF9ab89a), x + whiteW * 0.5f, whiteH,
                    false);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (keyRect, 3.0f);
            }

            // Borde de la tecla blanca
            g.setColour (juce::Colour (0xFF1a4a1a));
            g.drawRoundedRectangle (keyRect, 3.0f, 1.0f);

            // Etiqueta de nombre de nota (solo C de cada octava)
            if ((note % 12) == 0)
            {
                g.setColour (isActive ? juce::Colour (0xFF001a00)
                                      : juce::Colour (0xFF3a6a3a));
                g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
                g.drawText ("C" + juce::String (note / 12 - 1),
                            (int)(x), (int)(whiteH - 14), (int)(whiteW), 13,
                            juce::Justification::centred, false);
            }

            ++whiteIdx;
        }

        // -----------------------------------------------------------------
        // PASO 2: dibujar las teclas negras encima (para que tapen el borde
        // de las blancas adyacentes, igual que en un piano real)
        // -----------------------------------------------------------------
        whiteIdx = 0;
        for (int note = rangeStart; note <= rangeEnd; ++note)
        {
            if (isBlackKey (note))
            {
                // La tecla negra se centra entre la blanca anterior y la
                // siguiente, por eso usamos (whiteIdx - 0.5) como posicion.
                const float cx   = (whiteIdx - 0.5f) * whiteW;
                const float bx   = cx - blackW * 0.5f;
                juce::Rectangle<float> blackRect (bx + 1.0f, 0.0f, blackW - 2.0f, blackH);

                const bool isActive = (note == activeNote);

                if (isActive)
                {
                    juce::ColourGradient grad (
                        juce::Colour (0xFF00ff66), cx, 0.0f,
                        juce::Colour (0xFF003311), cx, blackH,
                        false);
                    g.setGradientFill (grad);
                    g.fillRoundedRectangle (blackRect, 2.0f);

                    g.setColour (juce::Colour (0x8800c853));
                    g.drawRoundedRectangle (blackRect.expanded (1.5f), 2.0f, 2.0f);
                }
                else
                {
                    juce::ColourGradient grad (
                        juce::Colour (0xFF1a2a1a), cx, 0.0f,
                        juce::Colour (0xFF060e06), cx, blackH,
                        false);
                    g.setGradientFill (grad);
                    g.fillRoundedRectangle (blackRect, 2.0f);
                }

                g.setColour (juce::Colour (0xFF0a1f0a));
                g.drawRoundedRectangle (blackRect, 2.0f, 1.0f);
            }
            else
            {
                ++whiteIdx;
            }
        }
    }

private:
    int rangeStart;
    int rangeEnd;
    int activeNote = -1;

    // Devuelve true si la nota (posicion dentro de la octava) es tecla negra.
    // Patron de teclas negras en una octava: 1, 3, 6, 8, 10 (Do#, Re#, Fa#, Sol#, La#)
    static bool isBlackKey (int midiNote)
    {
        const int pos = midiNote % 12;
        return (pos == 1 || pos == 3 || pos == 6 || pos == 8 || pos == 10);
    }

    // Cuenta cuantas teclas blancas hay en el rango [start, end] inclusive
    static int countWhiteKeys (int start, int end)
    {
        int count = 0;
        for (int n = start; n <= end; ++n)
            if (!isBlackKey (n)) ++count;
        return count;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboardDisplay)
};


//==============================================================================
// FRACTALIS AUDIO PROCESSOR EDITOR
//==============================================================================
class FractalisAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    FractalisAudioProcessorEditor (FractalisAudioProcessor&);
    ~FractalisAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    // =========================================================================
    // TIMER CALLBACK
    //
    // Se llama cada ~30ms (≈33fps) para leer lastMidiNote del procesador
    // y actualizar el teclado visual sin necesidad de callbacks desde el
    // hilo de audio, lo que seria inseguro en la GUI.
    // =========================================================================
    void timerCallback() override
    {
        const int note = audioProcessor.lastMidiNote.load();
        pianoDisplay.setActiveNote (note);
    }

    FractalisAudioProcessor& audioProcessor;

    // =========================================================================
    // TECLADO DE PIANO
    //
    // Componente de visualizacion MIDI. Muestra 3 octavas (C2-B4, notas 36-71)
    // para cubrir el rango mas comun de melodias y bajos.
    // Se posiciona en la franja inferior de la ventana.
    // =========================================================================
    PianoKeyboardDisplay pianoDisplay { 36, 71 };

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