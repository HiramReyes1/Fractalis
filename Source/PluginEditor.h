/*
  ==============================================================================
    PluginEditor.h — Fractalis Synthesizer v0.2

    Novedades respecto a v0.1:
      - Panel LFO   (izquierda): Rate, Depth, Wave, Dest
      - Panel ModEnv (centro) : ADSR + Amount + Dest
      - Panel Filter (derecha): Cutoff, Resonance, Mode
    Los tres paneles ocupan una segunda fila bajo los osciladores.
    La ventana crece de 560 a 680 px para alojar la fila extra.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// LOOK AND FEEL PERSONALIZADO PARA CONTROLAR SLIDERS
class FractalisLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider &slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2);
        auto radius = juce::jmin(bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f);
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // === AJUSTA ESTOS DOS VALORES A TU GUSTO ===
        const float arcThickness = 2.0f; // Grosor de la línea del arco
        const float thumbRadius = 2.5f;  // Radio del circulito thumb

        // --- Arco de fondo (outline) ---
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                    rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(slider.findColour(juce::Slider::rotarySliderOutlineColourId));
        g.strokePath(backgroundArc, juce::PathStrokeType(arcThickness));

        // --- Arco de valor (fill) ---
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f,
                                   rotaryStartAngle, angle, true);
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.strokePath(valueArc, juce::PathStrokeType(arcThickness));
        }

        // --- Thumb (circulito en la punta) ---
        float thumbX = centreX + radius * std::cos(angle - juce::MathConstants<float>::halfPi);
        float thumbY = centreY + radius * std::sin(angle - juce::MathConstants<float>::halfPi);
        juce::Point<float> thumbPoint(thumbX, thumbY);

        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillEllipse(juce::Rectangle<float>(thumbRadius * 2.0f, thumbRadius * 2.0f)
                          .withCentre(thumbPoint));
    }
};

// =============================================================================
// PIANO KEYBOARD DISPLAY
// =============================================================================
class PianoKeyboardDisplay : public juce::Component
{
public:
    // Callback: parámetros (midiNote, isNoteOn)
    std::function<void(int, bool)> onNoteEvent;

    PianoKeyboardDisplay(int firstNote = 36, int lastNote = 71)
        : rangeStart(firstNote), rangeEnd(lastNote) {}

    void setActiveNotes(const std::array<bool, 128> &notes)
    {
        if (activeNotes != notes)
        {
            activeNotes = notes;
            repaint();
        }
    }

    // ------------------------------------------------------------------
    // Mouse — click y arrastre al estilo Serum
    // ------------------------------------------------------------------
    void mouseDown(const juce::MouseEvent &e) override
    {
        const int note = getNoteAt(e.position);
        if (note >= 0)
        {
            currentDragNote = note;
            if (onNoteEvent)
                onNoteEvent(note, true);
        }
    }

    void mouseDrag(const juce::MouseEvent &e) override
    {
        const int note = getNoteAt(e.position);
        if (note != currentDragNote)
        {
            if (currentDragNote >= 0 && onNoteEvent)
                onNoteEvent(currentDragNote, false);
            currentDragNote = note;
            if (note >= 0 && onNoteEvent)
                onNoteEvent(note, true);
        }
    }

    void mouseUp(const juce::MouseEvent &) override
    {
        if (currentDragNote >= 0 && onNoteEvent)
        {
            onNoteEvent(currentDragNote, false);
            currentDragNote = -1;
        }
    }

    void mouseExit(const juce::MouseEvent &) override
    {
        if (currentDragNote >= 0 && onNoteEvent)
        {
            onNoteEvent(currentDragNote, false);
            currentDragNote = -1;
        }
    }

    // ------------------------------------------------------------------
    // Paint — idéntico al original pero usando activeNotes[]
    // ------------------------------------------------------------------
    void paint(juce::Graphics &g) override
    {
        const int numWhite = countWhiteKeys(rangeStart, rangeEnd);
        if (numWhite == 0)
            return;

        const float totalW = (float)getWidth();
        const float totalH = (float)getHeight();
        const float whiteW = totalW / (float)numWhite;
        const float blackW = whiteW * 0.58f;
        const float blackH = totalH * 0.60f;

        // Paso 1: teclas blancas
        int whiteIdx = 0;
        for (int note = rangeStart; note <= rangeEnd; ++note)
        {
            if (isBlackKey(note))
                continue;
            const float x = whiteIdx * whiteW;
            juce::Rectangle<float> keyRect(x + 1.0f, 0.0f, whiteW - 2.0f, totalH - 1.0f);
            const bool isActive = activeNotes[note];

            if (isActive)
            {
                juce::ColourGradient g2(juce::Colour(0xFF00ff66), x + whiteW * 0.5f, 0.0f,
                                        juce::Colour(0xFF004422), x + whiteW * 0.5f, totalH, false);
                g.setGradientFill(g2);
                g.fillRoundedRectangle(keyRect, 3.0f);
                g.setColour(juce::Colour(0x6600c853));
                g.drawRoundedRectangle(keyRect.expanded(1.5f), 3.0f, 2.0f);
            }
            else
            {
                juce::ColourGradient g2(juce::Colour(0xFFd8eed8), x + whiteW * 0.5f, 0.0f,
                                        juce::Colour(0xFF9ab89a), x + whiteW * 0.5f, totalH, false);
                g.setGradientFill(g2);
                g.fillRoundedRectangle(keyRect, 3.0f);
            }
            g.setColour(juce::Colour(0xFF1a4a1a));
            g.drawRoundedRectangle(keyRect, 3.0f, 1.0f);

            if ((note % 12) == 0)
            {
                g.setColour(isActive ? juce::Colour(0xFF001a00) : juce::Colour(0xFF3a6a3a));
                g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
                g.drawText("C" + juce::String(note / 12 - 1),
                           (int)x, (int)(totalH - 14), (int)whiteW, 13,
                           juce::Justification::centred, false);
            }
            ++whiteIdx;
        }

        // Paso 2: teclas negras
        whiteIdx = 0;
        for (int note = rangeStart; note <= rangeEnd; ++note)
        {
            if (isBlackKey(note))
            {
                const float cx = (whiteIdx - 0.5f) * whiteW;
                juce::Rectangle<float> blackRect(cx - blackW * 0.5f + 1.0f, 0.0f, blackW - 2.0f, blackH);
                const bool isActive = activeNotes[note];

                if (isActive)
                {
                    juce::ColourGradient g2(juce::Colour(0xFF00ff66), cx, 0.0f,
                                            juce::Colour(0xFF003311), cx, blackH, false);
                    g.setGradientFill(g2);
                    g.fillRoundedRectangle(blackRect, 2.0f);
                    g.setColour(juce::Colour(0x8800c853));
                    g.drawRoundedRectangle(blackRect.expanded(1.5f), 2.0f, 2.0f);
                }
                else
                {
                    juce::ColourGradient g2(juce::Colour(0xFF1a2a1a), cx, 0.0f,
                                            juce::Colour(0xFF060e06), cx, blackH, false);
                    g.setGradientFill(g2);
                    g.fillRoundedRectangle(blackRect, 2.0f);
                }
                g.setColour(juce::Colour(0xFF0a1f0a));
                g.drawRoundedRectangle(blackRect, 2.0f, 1.0f);
            }
            else
            {
                ++whiteIdx;
            }
        }
    }

private:
    int rangeStart, rangeEnd;
    std::array<bool, 128> activeNotes{};
    int currentDragNote = -1;

    // Devuelve el MIDI note en la posición del cursor, o -1 si no hay ninguna.
    // Las teclas negras tienen prioridad (están "encima" visualmente).
    int getNoteAt(juce::Point<float> pos) const
    {
        const int numWhite = countWhiteKeys(rangeStart, rangeEnd);
        if (numWhite == 0)
            return -1;

        const float totalW = (float)getWidth();
        const float totalH = (float)getHeight();
        const float whiteW = totalW / (float)numWhite;
        const float blackW = whiteW * 0.58f;
        const float blackH = totalH * 0.60f;

        // --- Teclas negras primero ---
        if (pos.y <= blackH)
        {
            int wIdx = 0;
            for (int note = rangeStart; note <= rangeEnd; ++note)
            {
                if (isBlackKey(note))
                {
                    const float cx = (wIdx - 0.5f) * whiteW;
                    const float left = cx - blackW * 0.5f + 1.0f;
                    const float right = left + blackW - 2.0f;
                    if (pos.x >= left && pos.x <= right)
                        return note;
                }
                else
                {
                    ++wIdx;
                }
            }
        }

        // --- Teclas blancas ---
        int wIdx = 0;
        for (int note = rangeStart; note <= rangeEnd; ++note)
        {
            if (isBlackKey(note))
                continue;
            const float left = wIdx * whiteW + 1.0f;
            const float right = left + whiteW - 2.0f;
            if (pos.x >= left && pos.x <= right)
                return note;
            ++wIdx;
        }

        return -1;
    }

    static bool isBlackKey(int note)
    {
        const int p = note % 12;
        return (p == 1 || p == 3 || p == 6 || p == 8 || p == 10);
    }
    static int countWhiteKeys(int start, int end)
    {
        int c = 0;
        for (int n = start; n <= end; ++n)
            if (!isBlackKey(n))
                ++c;
        return c;
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboardDisplay)
};

// =============================================================================
// FRACTALIS AUDIO PROCESSOR EDITOR
// =============================================================================
class FractalisAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    FractalisAudioProcessorEditor(FractalisAudioProcessor &);
    ~FractalisAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

private:
    FractalisLookAndFeel fractalisLookAndFeel;
    void timerCallback() override
    {
        const uint64_t bitsLow = audioProcessor.activeNoteBitsLow.load();
        const uint64_t bitsHigh = audioProcessor.activeNoteBitsHigh.load();

        std::array<bool, 128> held{};
        for (int n = 0; n < 64; ++n)
            held[n] = (bitsLow >> n) & 1ULL;
        for (int n = 0; n < 64; ++n)
            held[n + 64] = (bitsHigh >> n) & 1ULL;

        pianoDisplay.setActiveNotes(held);
    }

    FractalisAudioProcessor &audioProcessor;

    // Piano MIDI visual (C2–B4, 3 octavas)
    PianoKeyboardDisplay pianoDisplay{36, 71};

    // =========================================================================
    // CONTROLES OSC 1
    // =========================================================================
    juce::Label osc1Title;
    juce::ToggleButton osc1EnableButton;
    juce::Label osc1WaveLabel;
    juce::ComboBox osc1WaveCombo;

    juce::Label osc1VolumeLabel;
    juce::Slider osc1VolumeSlider;
    juce::Label osc1AttackLabel;
    juce::Slider osc1AttackSlider;
    juce::Label osc1DecayLabel;
    juce::Slider osc1DecaySlider;
    juce::Label osc1SustainLabel;
    juce::Slider osc1SustainSlider;
    juce::Label osc1ReleaseLabel;
    juce::Slider osc1ReleaseSlider;

    // =========================================================================
    // CONTROLES OSC 2
    // =========================================================================
    juce::Label osc2Title;
    juce::ToggleButton osc2EnableButton;
    juce::Label osc2WaveLabel;
    juce::ComboBox osc2WaveCombo;

    juce::Label osc2VolumeLabel;
    juce::Slider osc2VolumeSlider;
    juce::Label osc2AttackLabel;
    juce::Slider osc2AttackSlider;
    juce::Label osc2DecayLabel;
    juce::Slider osc2DecaySlider;
    juce::Label osc2SustainLabel;
    juce::Slider osc2SustainSlider;
    juce::Label osc2ReleaseLabel;
    juce::Slider osc2ReleaseSlider;

    // =========================================================================
    // CONTROLES LFO (panel izquierdo fila 2)
    // =========================================================================
    juce::Label lfoTitle;
    juce::Label lfoRateLabel;
    juce::Slider lfoRateSlider;
    juce::Label lfoDepthLabel;
    juce::Slider lfoDepthSlider;
    juce::Label lfoWaveLabel;
    juce::ComboBox lfoWaveCombo;
    juce::Label lfoDestLabel;
    juce::ComboBox lfoDestCombo;

    // =========================================================================
    // CONTROLES MOD ENV (panel central fila 2)
    // =========================================================================
    juce::Label modEnvTitle;
    juce::Label modEnvAttackLabel;
    juce::Slider modEnvAttackSlider;
    juce::Label modEnvDecayLabel;
    juce::Slider modEnvDecaySlider;
    juce::Label modEnvSustainLabel;
    juce::Slider modEnvSustainSlider;
    juce::Label modEnvReleaseLabel;
    juce::Slider modEnvReleaseSlider;
    juce::Label modEnvAmountLabel;
    juce::Slider modEnvAmountSlider;
    juce::Label modEnvDestLabel;
    juce::ComboBox modEnvDestCombo;

    // =========================================================================
    // CONTROLES FILTER (panel derecho fila 2)
    // =========================================================================
    juce::Label filterTitle;
    juce::Label filterCutoffLabel;
    juce::Slider filterCutoffSlider;
    juce::Label filterResLabel;
    juce::Slider filterResSlider;
    juce::Label filterModeLabel;
    juce::ComboBox filterModeCombo;

    // =========================================================================
    // ATTACHMENTS — conexiones GUI ↔ APVTS
    // =========================================================================

    // OSC 1
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> osc1EnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc1WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1VolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1AttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1DecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1SustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1ReleaseAttachment;

    // OSC 2
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> osc2EnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2VolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2AttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2DecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2SustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2ReleaseAttachment;

    // LFO
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoWaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoDestAttachment;

    // Mod Env
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modEnvAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modEnvDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modEnvSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modEnvReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modEnvAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modEnvDestAttachment;

    // Filter
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterModeAttachment;

    // =========================================================================
    // CONTROL DE VISTA — alterna entre sintetizador y efectos FX
    // =========================================================================
    enum class EditorView { Synth, Fx };
    EditorView currentView = EditorView::Synth;

    // Pestanas de navegacion en el header (siempre visibles)
    juce::TextButton viewSynthBtn;
    juce::TextButton viewFxBtn;

    // Listas de componentes por vista para control de visibilidad en bloque
    std::vector<juce::Component*> synthComponents;
    std::vector<juce::Component*> fxComponents;

    // =========================================================================
    // CONTROLES FX — paneles EQ, Compresor, Saturacion, Delay, Reverb
    // =========================================================================

    // --- EQ de 3 bandas ---
    juce::Label        fxEqTitle;
    juce::ToggleButton fxEqEnableBtn;
    juce::Label  fxEqLowLabel;    juce::Slider fxEqLowSlider;
    juce::Label  fxEqMidLabel;    juce::Slider fxEqMidSlider;
    juce::Label  fxEqHighLabel;   juce::Slider fxEqHighSlider;

    // --- Compresor ---
    juce::Label        fxCompTitle;
    juce::ToggleButton fxCompEnableBtn;
    juce::Label  fxCompThreshLabel;  juce::Slider fxCompThreshSlider;
    juce::Label  fxCompRatioLabel;   juce::Slider fxCompRatioSlider;
    juce::Label  fxCompAttackLabel;  juce::Slider fxCompAttackSlider;
    juce::Label  fxCompRelLabel;     juce::Slider fxCompRelSlider;
    juce::Label  fxCompMakeupLabel;  juce::Slider fxCompMakeupSlider;

    // --- Saturacion ---
    juce::Label        fxSatTitle;
    juce::ToggleButton fxSatEnableBtn;
    juce::Label  fxSatDriveLabel;    juce::Slider fxSatDriveSlider;
    juce::Label  fxSatMixLabel;      juce::Slider fxSatMixSlider;

    // --- Delay ---
    juce::Label        fxDelayTitle;
    juce::ToggleButton fxDelayEnableBtn;
    juce::Label  fxDelayTimeLabel;   juce::Slider fxDelayTimeSlider;
    juce::Label  fxDelayFbLabel;     juce::Slider fxDelayFbSlider;
    juce::Label  fxDelayMixLabel;    juce::Slider fxDelayMixSlider;

    // --- Reverb ---
    juce::Label        fxRevTitle;
    juce::ToggleButton fxRevEnableBtn;
    juce::Label  fxRevSizeLabel;     juce::Slider fxRevSizeSlider;
    juce::Label  fxRevDampLabel;     juce::Slider fxRevDampSlider;
    juce::Label  fxRevWidthLabel;    juce::Slider fxRevWidthSlider;
    juce::Label  fxRevMixLabel;      juce::Slider fxRevMixSlider;

    // =========================================================================
    // ATTACHMENTS FX
    // =========================================================================
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxEqEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxEqLowAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxEqMidAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxEqHighAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxCompEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxCompThreshAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxCompRatioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxCompAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxCompRelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxCompMakeupAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxSatEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxSatDriveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxSatMixAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxDelayEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDelayTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDelayFbAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxDelayMixAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fxRevEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxRevSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxRevDampAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxRevWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fxRevMixAttachment;

    // =========================================================================
    // HELPERS DE ESTILO (FX-especificos) Y GESTION DE VISTA
    // =========================================================================
    void setupEnableButton   (juce::ToggleButton&, const juce::String& text);
    void setupFxTitle        (juce::Label&,        const juce::String& text);
    void updateViewVisibility();

    // =========================================================================
    // HELPERS DE ESTILO
    // =========================================================================
    void setupKnob(juce::Slider &, bool bipolar = false);
    void setupLabel(juce::Label &, const juce::String &text);
    void setupCombo(juce::ComboBox &);
    void setupTitle(juce::Label &, const juce::String &text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FractalisAudioProcessorEditor)
};