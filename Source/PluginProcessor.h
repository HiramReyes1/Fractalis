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
    Count // siempre al final, útil para UI
};

// Nombres legibles para el ComboBox de destino
static const juce::StringArray kModDestNames{
    "None",
    "Osc1 Vol", "Osc2 Vol",
    "Osc1 Atk", "Osc1 Dec", "Osc1 Sus", "Osc1 Rel",
    "Osc2 Atk", "Osc2 Dec", "Osc2 Sus", "Osc2 Rel",
    "Flt Cutoff", "Flt Res"};

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

    void setSampleRate(double sr) noexcept { sampleRate = sr; }

    void setParameters(const Parameters &p) noexcept
    {
        params = p;
        attackRate = 1.0f / (juce::jmax(params.attack, 0.001f) * (float)sampleRate);
        decayRate = (1.0f - params.sustain) / (juce::jmax(params.decay, 0.001f) * (float)sampleRate);
    }

    void noteOn() noexcept
    {
        envelopeVal = 0.0f;
        state = State::Attack;
    }

    // Release calculado desde el valor ACTUAL del envelope — garantiza duración exacta.
    void noteOff() noexcept
    {
        if (state == State::Idle)
            return;
        if (envelopeVal <= 0.0f)
        {
            reset();
            return;
        }

        const float t = juce::jmax(params.release, 0.001f);
        releaseRate = envelopeVal / (t * (float)sampleRate);
        state = State::Release;
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
            if (envelopeVal <= 0.0f)
            {
                reset();
                return 0.0f;
            }
            break;
        default:
            return 0.0f;
        }
        return envelopeVal;
    }

    bool isActive() const noexcept { return state != State::Idle; }
    float getValue() const noexcept { return envelopeVal; }

    void reset() noexcept
    {
        envelopeVal = 0.0f;
        state = State::Idle;
    }

private:
    enum class State
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    double sampleRate = 44100.0;
    Parameters params;
    State state = State::Idle;
    float envelopeVal = 0.0f;
    float attackRate = 0.0f;
    float decayRate = 0.0f;
    float releaseRate = 0.0f; // Solo se escribe en noteOff()
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

    void setSampleRate(double sr) noexcept { sampleRate = sr; }

    // Recalcula coeficientes. Llamar una vez por bloque (no por muestra).
    // cutoffHz rango: 20–20000 Hz  |  resonance rango: 0.1–10
    void setParameters(float cutoffHz, float resonance) noexcept
    {
        const float clampedCutoff = juce::jlimit(20.0f, 20000.0f, cutoffHz);
        const float clampedRes = juce::jlimit(0.1f, 10.0f, resonance);

        // g = tan(π * fc / fs) — coeficiente de integración
        g = std::tan(juce::MathConstants<float>::pi * clampedCutoff / (float)sampleRate);
        // k = 1/Q donde Q ≈ resonance
        k = 1.0f / clampedRes;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Procesa una muestra. Devuelve la salida según el modo activo.
    float processSample(float input) noexcept
    {
        const float v3 = input - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        switch (mode)
        {
        case 0:
            return v2; // LP
        case 1:
            return input - k * v1 - v2; // HP
        case 2:
            return v1; // BP
        case 3:
            return input - k * v1; // Notch
        default:
            return v2;
        }
    }

    void setMode(int m) noexcept { mode = juce::jlimit(0, 3, m); }

    void reset() noexcept { ic1eq = ic2eq = 0.0f; }

private:
    double sampleRate = 44100.0;
    float g = 0.0f, k = 0.0f;
    float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
    float ic1eq = 0.0f, ic2eq = 0.0f; // variables de estado internas
    int mode = 0;
};

// =============================================================================
// SIMPLE DELAY — linea de retardo estereo con retroalimentacion
// Rango de tiempo: 0.01 s – 2.0 s. Pertenece a la cadena de efectos FX.
// =============================================================================
class SimpleDelay
{
public:
    SimpleDelay() = default;

    // Inicializa los buffers internos con el sample rate del host.
    // Llamar desde prepareToPlay() antes del primer uso.
    void prepare (double sampleRate) noexcept
    {
        sampleRateVal = sampleRate;
        const int maxSamples = static_cast<int>(2.0 * sampleRate) + 64;
        bufferL.assign (maxSamples, 0.0f);
        bufferR.assign (maxSamples, 0.0f);
        writePos = 0;
    }

    // Procesa numSamples estereo en los punteros left/right (in-place).
    // delayTimeSec : retardo en segundos  (0.01 – 2.0)
    // feedback     : retroalimentacion   (0.0 – 0.95)
    // mix          : proporcion de senal retardada en la salida (0.0 – 1.0)
    void process (float* left, float* right, int numSamples,
                  float delayTimeSec, float feedback, float mix) noexcept
    {
        if (bufferL.empty()) return;
        const int bufSize = static_cast<int>(bufferL.size());
        const int delayN  = juce::jlimit (1, bufSize - 1,
                            static_cast<int>(delayTimeSec * sampleRateVal));

        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (writePos - delayN + bufSize) % bufSize;
            const float dL = bufferL[readPos];
            const float dR = bufferR[readPos];

            // Grabar en el buffer: senal actual + retorno con feedback
            bufferL[writePos] = left[i]  + dL * feedback;
            bufferR[writePos] = right[i] + dR * feedback;

            // Salida: senal seca + senal retardada escalada por mix
            left[i]  += dL * mix;
            right[i] += dR * mix;

            writePos = (writePos + 1) % bufSize;
        }
    }

    void reset() noexcept
    {
        std::fill (bufferL.begin(), bufferL.end(), 0.0f);
        std::fill (bufferR.begin(), bufferR.end(), 0.0f);
        writePos = 0;
    }

private:
    double sampleRateVal = 44100.0;
    std::vector<float> bufferL, bufferR;
    int writePos = 0;
};

// =============================================================================
// SIMPLE COMPRESSOR — compresor de pico con seguidor de envolvente logaritmico
// Trabaja en dB internamente para mayor linealidad perceptual.
// Crear una instancia por canal para procesado estereo independiente.
// =============================================================================
class SimpleCompressor
{
public:
    SimpleCompressor() = default;

    // Llamar desde prepareToPlay() con el sample rate del host.
    void prepare (double sampleRate) noexcept
    {
        sampleRateVal = sampleRate;
        updateCoefficients();
    }

    // Actualiza parametros. Llamar una vez por bloque si cambian los valores.
    // thresholdDb : umbral de compresion en dB  (-60 a 0)
    // ratio       : ratio de compresion (1 = sin compresion, 20 = limitador)
    // attackMs    : tiempo de ataque en milisegundos
    // releaseMs   : tiempo de recuperacion en milisegundos
    // makeupDb    : ganancia de compensacion en dB aplicada a la salida
    void setParameters (float thresholdDb, float ratio,
                        float attackMs, float releaseMs,
                        float makeupDb) noexcept
    {
        threshDb  = thresholdDb;
        compRatio = juce::jmax (1.0f, ratio);
        atkMs     = juce::jmax (0.1f, attackMs);
        relMs     = juce::jmax (1.0f, releaseMs);
        makeupLin = juce::Decibels::decibelsToGain (makeupDb);
        updateCoefficients();
    }

    // Procesa una muestra y devuelve la salida comprimida + makeup gain.
    float processSample (float input) noexcept
    {
        const float absIn = std::abs (input);
        const float inDb  = absIn > 1e-7f
                            ? juce::Decibels::gainToDecibels (absIn) : -140.0f;

        // Seguidor de envolvente: rama attack si la senal sube, release si baja
        envelope = (inDb > envelope)
                   ? atk * envelope + (1.0f - atk) * inDb
                   : rel * envelope + (1.0f - rel) * inDb;

        // Reduccion de ganancia cuando la envolvente supera el umbral
        const float gainDb = (envelope > threshDb)
                             ? (envelope - threshDb) * (1.0f / compRatio - 1.0f)
                             : 0.0f;

        return input * juce::Decibels::decibelsToGain (gainDb) * makeupLin;
    }

    void reset() noexcept { envelope = -140.0f; }

private:
    void updateCoefficients() noexcept
    {
        atk = std::exp (-1.0f / static_cast<float>(atkMs * 0.001 * sampleRateVal));
        rel = std::exp (-1.0f / static_cast<float>(relMs * 0.001 * sampleRateVal));
    }

    double sampleRateVal = 44100.0;
    float  threshDb      = -20.0f;
    float  compRatio     =   4.0f;
    float  atkMs         =  10.0f;
    float  relMs         = 100.0f;
    float  makeupLin     =   1.0f;
    float  atk           =   0.0f;
    float  rel           =   0.0f;
    float  envelope      = -140.0f;
};

// =============================================================================
// SYNTH VOICE — estado completo de una voz polifónica
// =============================================================================
struct SynthVoice
{
    int midiNote = -1;
    double frequency = 440.0;
    double phaseInc = 0.0;
    double osc1Phase = 0.0;
    double osc2Phase = 0.0;
    FixedADSR adsr1;
    FixedADSR adsr2;

    bool isActive() const noexcept { return adsr1.isActive(); }

    void reset() noexcept
    {
        midiNote = -1;
        osc1Phase = 0.0;
        osc2Phase = 0.0;
        adsr1.reset();
        adsr2.reset();
    }
};

// =============================================================================
// FRACTALIS AUDIO PROCESSOR
// =============================================================================
class FractalisAudioProcessor : public juce::AudioProcessor
{
public:
    FractalisAudioProcessor();
    ~FractalisAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // -------------------------------------------------------------------------
    // APVTS — sistema central de parámetros
    //
    // Parámetros OSC 1/2 (7 c/u), Filtro (3), LFO (4), ModEnv (6)
    // -------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Comunicación hilo de audio → GUI (leer nota activa para el teclado)
    std::atomic<int> lastMidiNote{-1};

    // Bitmask thread-safe: cada bit = una nota MIDI activa (física, no en release)
    std::atomic<uint64_t> activeNoteBitsLow{0};  // notas 0–63
    std::atomic<uint64_t> activeNoteBitsHigh{0}; // notas 64–127

    // Inyecta MIDI desde la GUI (click en el teclado piano)
    void addGuiMidiMessage(const juce::MidiMessage &msg);

private:
    juce::CriticalSection guiMidiLock;
    juce::Array<juce::MidiMessage> guiMidiQueue;
    // =========================================================================
    // VOCES POLIFÓNICAS
    // =========================================================================
    static constexpr int kMaxVoices = 8;
    SynthVoice voices[kMaxVoices];

    // =========================================================================
    // HELPER PRIVADO
    // =========================================================================
    SynthVoice *findFreeVoice() noexcept;

    // Fase del LFO (0.0–1.0), avanza independientemente de notas
    double lfoPhase = 0.0;

    // =========================================================================
    // MÓDULOS DE AUDIO
    // =========================================================================

    FixedADSR modEnv; // Envelope de modulación (no afecta audio directamente)
    SVFilter filter;  // Filtro State Variable — procesa la salida mezclada

    // =========================================================================
    // GENERADORES DE ONDA
    // =========================================================================
    float generateWave(double phase, int waveType);
    float generateSine(double phase);
    float generateSaw(double phase);
    float generateSquare(double phase);
    float generateTriangle(double phase);

    // =========================================================================
    // MODULACIÓN
    //
    // applyModulation() recibe el valor normalizado del modulador (−1 a +1 para
    // el LFO, 0 a +1 para ModEnv) y lo suma al parámetro destino dentro del
    // rango válido de ese parámetro.  Devuelve el valor modulado final.
    // =========================================================================
    float applyModulation(ModDestination dest, float baseValue, float modValue);

    double midiNoteToFrequency(int midiNote);

    // =========================================================================
    // CADENA DE EFECTOS (FX)
    // Orden de aplicacion en processBlock:
    //   EQ de 3 bandas -> Compresor -> Saturacion -> Delay -> Reverb
    // =========================================================================

    // EQ: un filtro IIR por banda (grave / medio / agudo) por canal (L y R)
    juce::IIRFilter  eqLowL,  eqLowR;    // Shelving grave  — fc fija: 300 Hz
    juce::IIRFilter  eqMidL,  eqMidR;    // Pico medio      — fc fija: 1 kHz
    juce::IIRFilter  eqHighL, eqHighR;   // Shelving agudo  — fc fija: 5 kHz

    // Compresor: canales izquierdo y derecho procesados independientemente
    SimpleCompressor fxCompL, fxCompR;

    // Delay estereo con retroalimentacion
    SimpleDelay fxDelay;

    // Reverb — juce::Reverb tiene API directa (no requiere ProcessSpec)
    juce::Reverb fxReverb;

    // Sample rate almacenado para recalcular coeficientes de EQ en processBlock
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FractalisAudioProcessor)
};