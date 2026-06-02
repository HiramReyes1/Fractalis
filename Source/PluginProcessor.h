/*
  ==============================================================================
    PluginProcessor.h
    Aquí DECLARAMOS todo lo que el procesador de audio va a usar.
    Piénsalo como el "índice" o "contrato" de lo que existe.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

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
    // PARÁMETROS DEL OSCILADOR
    // AudioProcessorValueTreeState (APVTS) es el sistema de JUCE para manejar
    // parámetros que se pueden automatizar en el DAW, guardar en presets, y
    // conectar automáticamente a controles de la GUI.
    // =========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Esta función estática crea el "layout" (lista) de todos los parámetros.
    // Es estática porque la necesitamos antes de que el procesador exista.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    // =========================================================================
    // VARIABLES INTERNAS DEL OSCILADOR
    // =========================================================================

    // La fase es la posición actual en el ciclo de la onda (0.0 a 1.0)
    // Necesitamos una por canal (izquierdo y derecho) para audio estéreo
    double phase = 0.0;

    // El incremento de fase: cuánto avanzamos por sample para generar
    // la frecuencia correcta. Se calcula como: frecuencia / sampleRate
    double phaseIncrement = 0.0;

    // La frecuencia MIDI actual (en Hz). Se actualiza con cada noteOn.
    double currentFrequency = 440.0;

    // Si hay una nota activa o no (para silenciar cuando no hay nota)
    bool noteIsActive = false;

    // La velocidad (velocity) de la nota MIDI (0.0 a 1.0)
    // Controla el volumen de la nota
    float currentVelocity = 0.0f;

    // =========================================================================
    // FUNCIONES GENERADORAS DE ONDA
    // Cada una recibe la fase actual (0.0 a 1.0) y devuelve una muestra
    // de audio (-1.0 a 1.0)
    // =========================================================================

    // Onda sinusoidal - suave, sin armónicos
    float generateSine (double currentPhase);

    // Onda de sierra (sawtooth) - rica en armónicos, sonido brillante
    float generateSaw (double currentPhase);

    // Onda cuadrada (square) - armónicos impares, sonido hueco/nasal
    float generateSquare (double currentPhase);

    // Onda triangular - armónicos impares pero más suave que la cuadrada
    float generateTriangle (double currentPhase);

    // Convierte un número de nota MIDI (0-127) a frecuencia en Hz
    // Fórmula: 440 * 2^((nota - 69) / 12)
    double midiNoteToFrequency (int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FractalisAudioProcessor)
};