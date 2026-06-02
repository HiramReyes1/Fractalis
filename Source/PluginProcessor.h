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

private:
    // =========================================================================
    // ESTADO DE LA NOTA MIDI
    //
    // Ambos osciladores comparten la misma nota MIDI (tocan al unsono).
    // El phaseIncrement se calcula una sola vez al recibir el noteOn.
    // =========================================================================
    double currentFrequency = 440.0;
    double phaseIncrement   = 0.0;
    bool   noteIsActive     = false;
    float  currentVelocity  = 0.0f;

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
    // juce::ADSR es el procesador de envelope incluido en JUCE.
    // Cada oscilador tiene el suyo propio para permitir configuraciones
    // ADSR diferentes (por ejemplo, OSC 1 con ataque corto, OSC 2 con
    // ataque largo para una capa de pad).
    //
    // El ADSR tiene cuatro etapas:
    //   Attack  (ATK): tiempo que tarda en llegar de 0 a 1 al presionar la tecla
    //   Decay   (DCY): tiempo que tarda en bajar de 1 al nivel de sustain
    //   Sustain (SUS): nivel que mantiene mientras la tecla esta presionada
    //   Release (REL): tiempo que tarda en llegar a 0 al soltar la tecla
    // =========================================================================
    juce::ADSR adsr1;
    juce::ADSR adsr2;

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