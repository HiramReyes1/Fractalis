/*
  ==============================================================================
    PluginProcessor.h — Fractalis Synthesizer v0.2

    Nuevos módulos añadidos respecto a v0.1:
      - SVFilter    : filtro State Variable (LP/HP/BP/Notch), 2do orden
      - LFO         : oscilador de baja frecuencia con destino de modulación
      - ModEnv      : envelope de modulación independiente (no ligado a audio)
      - Sistema de routing: LFO y ModEnv pueden apuntar a cualquier parámetro
                            continuo del sintetizador vía un índice de destino.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// =============================================================================
// CONSTANTES DE MODULACIÓN
//
// Índices de destino compartidos por LFO y ModEnv.
// Añadir un nuevo destino = añadir una entrada aquí + mapearlo en
// FractalisAudioProcessor::applyModulation().
// =============================================================================
enum class ModDestination : int
{
    None = 0,
    // --- Volúmenes de osciladores ---
    Osc1Volume,
    Osc2Volume,
    // --- ADSR OSC 1 ---
    Osc1Attack,
    Osc1Decay,
    Osc1Sustain,
    Osc1Release,
    // --- ADSR OSC 2 ---
    Osc2Attack,
    Osc2Decay,
    Osc2Sustain,
    Osc2Release,
    // --- Filtro ---
    FilterCutoff,
    FilterResonance,
    Count  // siempre al final, útil para UI
};

// Nombres legibles para el ComboBox de destino
static const juce::StringArray kModDestNames {
    "None",
    "Osc1 Vol", "Osc2 Vol",
    "Osc1 Atk", "Osc1 Dec", "Osc1 Sus", "Osc1 Rel",
    "Osc2 Atk", "Osc2 Dec", "Osc2 Sus", "Osc2 Rel",
    "Flt Cutoff", "Flt Res"
};

// =============================================================================
// FixedADSR — ADSR con release calibrado desde el valor actual del envelope
//
// Corrige el bug de juce::ADSR donde el release dura más de lo configurado
// cuando noteOff llega durante el attack o decay (antes de llegar al sustain).
// =============================================================================
class FixedADSR
{
public:
    FixedADSR() = default;
    using Parameters = juce::ADSR::Parameters;

    void setSampleRate (double sr) noexcept { sampleRate = sr; }

    void setParameters (const Parameters& p) noexcept
    {
        params     = p;
        attackRate = 1.0f / (juce::jmax (params.attack,  0.001f) * (float) sampleRate);
        decayRate  = (1.0f - params.sustain)
                     / (juce::jmax (params.decay, 0.001f) * (float) sampleRate);
    }

    void noteOn() noexcept
    {
        envelopeVal = 0.0f;
        state       = State::Attack;
    }

    // Release calculado desde el valor ACTUAL del envelope — garantiza duración exacta.
    void noteOff() noexcept
    {
        if (state == State::Idle) return;
        if (envelopeVal <= 0.0f) { reset(); return; }

        const float t = juce::jmax (params.release, 0.001f);
        releaseRate   = envelopeVal / (t * (float) sampleRate);
        state         = State::Release;
    }

    float getNextSample() noexcept
    {
        switch (state)
        {
            case State::Attack:
                envelopeVal += attackRate;
                if (envelopeVal >= 1.0f)
                {
                    envelopeVal = 1.0f;
                    state = (params.sustain < 1.0f) ? State::Decay : State::Sustain;
                }
                break;
            case State::Decay:
                envelopeVal -= decayRate;
                if (envelopeVal <= params.sustain)
                {
                    envelopeVal = params.sustain;
                    state = State::Sustain;
                }
                break;
            case State::Sustain:
                envelopeVal = params.sustain;
                break;
            case State::Release:
                envelopeVal -= releaseRate;
                if (envelopeVal <= 0.0f) { reset(); return 0.0f; }
                break;
            default:
                return 0.0f;
        }
        return envelopeVal;
    }

    bool  isActive() const noexcept { return state != State::Idle; }
    float getValue() const noexcept { return envelopeVal; }

    void reset() noexcept
    {
        envelopeVal = 0.0f;
        state       = State::Idle;
    }

private:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    double     sampleRate   = 44100.0;
    Parameters params;
    State      state        = State::Idle;
    float      envelopeVal  = 0.0f;
    float      attackRate   = 0.0f;
    float      decayRate    = 0.0f;
    float      releaseRate  = 0.0f;  // Solo se escribe en noteOff()
};


// =============================================================================
// SVFilter — State Variable Filter de 2do orden
//
// Implementación de Zavalishin (The Art of VA Filter Design, 2018).
// Produce simultáneamente LP, HP, BP y Notch sin cambiar coeficientes.
// Modos: 0=LP, 1=HP, 2=BP, 3=Notch
//
// Variables de estado (ic1eq, ic2eq) persisten entre bloques para continuidad.
// =============================================================================
class SVFilter
{
public:
    SVFilter() = default;

    void setSampleRate (double sr) noexcept { sampleRate = sr; }

    // Recalcula coeficientes. Llamar una vez por bloque (no por muestra).
    // cutoffHz rango: 20–20000 Hz  |  resonance rango: 0.1–10
    void setParameters (float cutoffHz, float resonance) noexcept
    {
        const float clampedCutoff = juce::jlimit (20.0f, 20000.0f, cutoffHz);
        const float clampedRes    = juce::jlimit (0.1f,  10.0f,    resonance);

        // g = tan(π * fc / fs) — coeficiente de integración
        g  = std::tan (juce::MathConstants<float>::pi * clampedCutoff / (float) sampleRate);
        // k = 1/Q donde Q ≈ resonance
        k  = 1.0f / clampedRes;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Procesa una muestra. Devuelve la salida según el modo activo.
    float processSample (float input) noexcept
    {
        const float v3 = input - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        switch (mode)
        {
            case 0: return v2;              // LP
            case 1: return input - k*v1 - v2; // HP
            case 2: return v1;              // BP
            case 3: return input - k*v1;    // Notch
            default: return v2;
        }
    }

    void setMode (int m) noexcept { mode = juce::jlimit (0, 3, m); }

    void reset() noexcept { ic1eq = ic2eq = 0.0f; }

private:
    double sampleRate = 44100.0;
    float  g = 0.0f, k = 0.0f;
    float  a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float  ic1eq = 0.0f, ic2eq = 0.0f;  // variables de estado internas
    int    mode = 0;
};


// =============================================================================
// FRACTALIS AUDIO PROCESSOR
// =============================================================================
class FractalisAudioProcessor : public juce::AudioProcessor
{
public:
    FractalisAudioProcessor();
    ~FractalisAudioProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool   acceptsMidi()  const override;
    bool   producesMidi() const override;
    bool   isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms()  override;
    int  getCurrentProgram() override;
    void setCurrentProgram  (int index) override;
    const juce::String getProgramName    (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // -------------------------------------------------------------------------
    // APVTS — sistema central de parámetros
    //
    // Parámetros OSC 1/2 (7 c/u), Filtro (3), LFO (4), ModEnv (6)
    // -------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Comunicación hilo de audio → GUI (leer nota activa para el teclado)
    std::atomic<int> lastMidiNote { -1 };

private:
    // =========================================================================
    // ESTADO DE NOTA MIDI
    // =========================================================================
    double currentFrequency = 440.0;
    double phaseIncrement   = 0.0;
    bool   noteIsActive     = false;
    int    heldNoteCount    = 0;

    // Acumuladores de fase (0.0–1.0) de ambos osciladores
    double osc1Phase = 0.0;
    double osc2Phase = 0.0;

    // Fase del LFO (0.0–1.0), avanza independientemente de notas
    double lfoPhase = 0.0;

    // =========================================================================
    // MÓDULOS DE AUDIO
    // =========================================================================
    FixedADSR adsr1;    // Envelope oscilador 1
    FixedADSR adsr2;    // Envelope oscilador 2
    FixedADSR modEnv;   // Envelope de modulación (no afecta audio directamente)
    SVFilter  filter;   // Filtro State Variable — procesa la salida mezclada

    // =========================================================================
    // GENERADORES DE ONDA
    // =========================================================================
    float  generateWave     (double phase, int waveType);
    float  generateSine     (double phase);
    float  generateSaw      (double phase);
    float  generateSquare   (double phase);
    float  generateTriangle (double phase);

    // =========================================================================
    // MODULACIÓN
    //
    // applyModulation() recibe el valor normalizado del modulador (−1 a +1 para
    // el LFO, 0 a +1 para ModEnv) y lo suma al parámetro destino dentro del
    // rango válido de ese parámetro.  Devuelve el valor modulado final.
    // =========================================================================
    float applyModulation (ModDestination dest, float baseValue, float modValue);

    double midiNoteToFrequency (int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractalisAudioProcessor)
};