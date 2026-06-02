/*
  ==============================================================================
    PluginProcessor.h

    Declara todos los parametros, variables internas y funciones del procesador.
    Este archivo es el "contrato" que otros archivos consultan para saber
    que existe y que puede hacer el procesador.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
// ADSR PERSONALIZADO (FixedADSR)
//
// Reemplaza juce::ADSR para corregir el calculo del release cuando noteOff
// ocurre durante el attack o el decay (antes de que el envelope llegue al
// nivel de sustain).
//
// PROBLEMA con juce::ADSR:
//   releaseRate se precalcula como  sustain / (release x sampleRate).
//   Eso esta calibrado para bajar DESDE el nivel sustain hasta 0.
//   Si noteOff llega mientras envelopeVal > sustain, el tiempo real es:
//       (envelopeVal / sustain) x release
//   Ejemplo: sustain = 0.18, release = 5 s, envelopeVal = 1.0  ->  27 s.
//   Tocar fuerte/rapido suelta la nota durante el attack, antes de que
//   el envelope baje al sustain, reproduciendo exactamente ese escenario.
//
// SOLUCION:
//   En noteOff(), releaseRate se calcula desde el valor ACTUAL del envelope.
//   Garantia: el fade dura exactamente params.release segundos sin importar
//   en que fase del ADSR se llame noteOff().
//==============================================================================
class FixedADSR
{
public:
    // Constructor por defecto explicito requerido por MSVC cuando la clase
    // se usa como miembro de valor (no en heap). No necesita leak detector
    // porque su lifetime esta ligado al FractalisAudioProcessor que si lo tiene.
    FixedADSR() = default;

    // Reutilizamos la estructura de parametros de JUCE para compatibilidad directa
    using Parameters = juce::ADSR::Parameters;

    void setSampleRate (double sr) noexcept
    {
        sampleRate = sr;
    }

    // Recalcula attack y decay.
    // NO toca releaseRate: se calcula en noteOff() a partir del valor actual.
    void setParameters (const Parameters& p) noexcept
    {
        params     = p;
        attackRate = 1.0f / (juce::jmax (params.attack, 0.001f) * (float) sampleRate);
        decayRate  = (1.0f - params.sustain)
                     / (juce::jmax (params.decay, 0.001f) * (float) sampleRate);
    }

    // noteOn siempre arranca desde cero (comportamiento clasico de sintetizador)
    void noteOn() noexcept
    {
        envelopeVal = 0.0f;
        state       = State::Attack;
    }

    // noteOff calcula releaseRate desde envelopeVal en este instante exacto.
    // Garantia: el fade a 0 tarda exactamente params.release segundos,
    // sin importar si noteOff llego durante attack, decay o sustain.
    void noteOff() noexcept
    {
        if (state == State::Idle)
            return;

        // Si el envelope ya esta en 0 (nota soltada antes de empezar),
        // ir directo a idle sin calcular rates.
        if (envelopeVal <= 0.0f)
        {
            reset();
            return;
        }

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
                    // Omitir decay si sustain ya esta en 1.0
                    state = (params.sustain < 1.0f) ? State::Decay : State::Sustain;
                }
                break;

            case State::Decay:
                envelopeVal -= decayRate;
                if (envelopeVal <= params.sustain)
                {
                    envelopeVal = params.sustain;
                    state       = State::Sustain;
                }
                break;

            case State::Sustain:
                // Mantener el nivel de sustain mientras la nota este presionada
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
    float      releaseRate  = 0.0f;  // Solo se escribe en noteOff(), nunca en setParameters()
};
// FIN de FixedADSR


//==============================================================================
// FRACTALIS AUDIO PROCESSOR
//==============================================================================
class FractalisAudioProcessor : public juce::AudioProcessor
{
public:
    FractalisAudioProcessor();
    ~FractalisAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // =========================================================================
    // APVTS (AudioProcessorValueTreeState)
    //
    // Sistema central de parametros de JUCE. Maneja automaticamente:
    //   - Automatizacion en el DAW
    //   - Guardado y carga de presets
    //   - Sincronizacion bidireccional con controles de la GUI
    //
    // Parametros de OSC 1: osc1Enabled, osc1WaveType, osc1Volume,
    //                      osc1Attack, osc1Decay, osc1Sustain, osc1Release
    // Parametros de OSC 2: osc2Enabled, osc2WaveType, osc2Volume,
    //                      osc2Attack, osc2Decay, osc2Sustain, osc2Release
    // =========================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // =========================================================================
    // COMUNICACION MIDI -> GUI (thread-safe)
    //
    // El hilo de audio escribe aqui la nota activa; el hilo de GUI lo lee
    // periodicamente (via Timer) para animar el teclado sin bloqueos.
    //
    // Convencion de valores:
    //   -1  = ninguna nota activa (noteOff o allNotesOff)
    //   0-127 = numero de nota MIDI actualmente sonando
    //
    // std::atomic garantiza lecturas/escrituras atomicas entre hilos sin
    // necesitar un mutex, lo cual es obligatorio en el hilo de audio.
    // =========================================================================
    std::atomic<int> lastMidiNote { -1 };

private:
    // =========================================================================
    // ESTADO DE LA NOTA MIDI
    //
    // Ambos osciladores comparten la misma nota MIDI (tocan al unsono).
    // El phaseIncrement se calcula una sola vez al recibir el noteOn.
    //
    // noteOnVelocity guarda la velocity del ultimo noteOn y NO se resetea
    // en noteOff, porque el ADSR necesita seguir escalando el audio durante
    // la etapa de Release. Ponerla a 0 en noteOff cortaria el release
    // instantaneamente antes de que el envelope pueda hacer su fade out.
    // =========================================================================
    double currentFrequency = 440.0;
    double phaseIncrement   = 0.0;
    bool   noteIsActive     = false;
    //float  noteOnVelocity   = 0.0f;

    // =========================================================================
    // ACUMULADORES DE FASE
    //
    // Cada oscilador tiene su propio acumulador de fase (0.0 a 1.0).
    // Representan la posicion actual dentro del ciclo de la onda.
    // Tenerlos separados permite que cada oscilador funcione de forma
    // independiente (por ejemplo, si en el futuro se agregan detuning
    // o formas de onda que se reinician de forma distinta).
    // =========================================================================
    double osc1Phase = 0.0;
    double osc2Phase = 0.0;

    // =========================================================================
    // ENVELOPES ADSR
    //
    // FixedADSR reemplaza juce::ADSR para garantizar que el release dure
    // exactamente el tiempo configurado, independientemente de la fase en
    // que se llame noteOff(). Ver la clase FixedADSR para el detalle tecnico.
    //
    // El ADSR tiene cuatro etapas:
    //   Attack  (ATK): tiempo que tarda en llegar de 0 a 1 al presionar la tecla
    //   Decay   (DCY): tiempo que tarda en bajar de 1 al nivel de sustain
    //   Sustain (SUS): nivel que mantiene mientras la tecla esta presionada
    //   Release (REL): tiempo que tarda en llegar a 0 al soltar la tecla
    // =========================================================================
    FixedADSR adsr1;
    FixedADSR adsr2;

    // =========================================================================
    // SEGUIMIENTO DE NOTAS PRESIONADAS
    //
    // Cuenta cuantas teclas MIDI estan actualmente presionadas.
    // El release del ADSR se dispara SOLO cuando este contador llega a 0,
    // evitando que acordes o notas solapadas reinicien el timer de release
    // con cada noteOff individual.
    // =========================================================================
    int heldNoteCount = 0;

    // =========================================================================
    // GENERADORES DE ONDA
    //
    // generateWave() es el dispatcher: recibe el tipo de onda (0-3) y
    // llama al generador correspondiente.
    //
    // Cada generador individual recibe la fase actual (0.0 a 1.0) y
    // devuelve una muestra de audio en el rango [-1.0, 1.0].
    // =========================================================================
    float generateWave     (double currentPhase, int waveType);
    float generateSine     (double currentPhase);
    float generateSaw      (double currentPhase);
    float generateSquare   (double currentPhase);
    float generateTriangle (double currentPhase);

    // Convierte un numero de nota MIDI (0-127) a frecuencia en Hz
    double midiNoteToFrequency (int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractalisAudioProcessor)
};