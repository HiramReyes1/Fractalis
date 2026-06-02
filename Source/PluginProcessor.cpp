/*
  ==============================================================================
    PluginProcessor.cpp
    Aquí está TODA la lógica de audio: generación de ondas, procesamiento
    de MIDI, y el corazón del sintetizador.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// CREAR PARÁMETROS
// Esta función define todos los parámetros que tendrá el sintetizador.
// Cada parámetro tiene: ID, nombre visible, rango, y valor por defecto.
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    FractalisAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Parámetro: Tipo de onda del oscilador
    // AudioParameterChoice = lista de opciones (no un número continuo)
    // Opciones: 0=Sine, 1=Saw, 2=Square, 3=Triangle
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "waveType",           // ID interno (para referenciarlo en el código)
        "Wave Type",          // Nombre visible en el DAW
        juce::StringArray { "Sine", "Saw", "Square", "Triangle" },
        0                     // Índice por defecto (0 = Sine)
    ));

    // Parámetro: Volumen general
    // AudioParameterFloat = valor continuo flotante
    // NormalisableRange: mínimo, máximo, incremento
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "volume",
        "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.7f                  // Valor por defecto: 70%
    ));

    return layout;
}

//==============================================================================
// CONSTRUCTOR
// Se llama una vez cuando el plugin se carga.
// Inicializamos el APVTS aquí, pasándole el layout de parámetros.
//==============================================================================
FractalisAudioProcessor::FractalisAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       // Inicializamos el APVTS con:
       // - *this: referencia a este procesador
       // - nullptr: no usamos UndoManager por ahora
       // - "Parameters": nombre del nodo raíz en el XML de estado
       // - createParameterLayout(): la lista de parámetros que definimos arriba
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

FractalisAudioProcessor::~FractalisAudioProcessor() {}

//==============================================================================
// PREPARE TO PLAY
// El DAW llama esto antes de empezar a reproducir audio.
// Aquí inicializamos todo lo que depende del sampleRate o el tamaño del buffer.
//==============================================================================
void FractalisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Por ahora no necesitamos hacer nada especial aquí.
    // Cuando agreguemos filtros y efectos, los inicializaremos aquí.
}

void FractalisAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FractalisAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
  #endif
}
#endif

//==============================================================================
// PROCESS BLOCK — EL CORAZÓN DEL PLUGIN
// Esta función se llama MILES de veces por segundo (cada ~5ms a 44100Hz).
// Aquí generamos cada muestra de audio y procesamos el MIDI.
//
// buffer: donde escribimos el audio de salida
// midiMessages: los mensajes MIDI que llegaron en este bloque
//==============================================================================
void FractalisAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Leer los parámetros actuales del APVTS
    // getRawParameterValue devuelve un puntero atómico al valor del parámetro
    auto* waveTypeParam = apvts.getRawParameterValue ("waveType");
    auto* volumeParam   = apvts.getRawParameterValue ("volume");

    int waveType = (int) waveTypeParam->load();  // 0=Sine, 1=Saw, 2=Square, 3=Triangle
    float volume = volumeParam->load();           // 0.0 a 1.0

    // Obtener punteros de escritura para cada canal de salida
    auto* leftChannel  = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);

    int numSamples = buffer.getNumSamples();

    // =========================================================================
    // PROCESAR MENSAJES MIDI
    // Iteramos por todos los eventos MIDI que llegaron en este bloque
    // =========================================================================
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            // noteOn: el usuario presionó una tecla
            noteIsActive = true;

            // Convertir número de nota MIDI a frecuencia en Hz
            currentFrequency = midiNoteToFrequency (message.getNoteNumber());

            // Velocity: qué tan fuerte se presionó la tecla (0-127 → 0.0-1.0)
            currentVelocity = message.getVelocity() / 127.0f;

            // Calcular el incremento de fase para esta frecuencia
            // Por cada sample, avanzamos esta fracción del ciclo completo
            phaseIncrement = currentFrequency / getSampleRate();
        }
        else if (message.isNoteOff())
        {
            // noteOff: el usuario soltó la tecla
            // (Más adelante aquí irá el release del ADSR)
            noteIsActive = false;
            currentVelocity = 0.0f;
        }
        else if (message.isAllNotesOff())
        {
            // Panic: silenciar todo
            noteIsActive = false;
            currentVelocity = 0.0f;
            phase = 0.0;
        }
    }

    // =========================================================================
    // GENERAR AUDIO SAMPLE POR SAMPLE
    // Iteramos por cada muestra del buffer y generamos el audio
    // =========================================================================
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float outputSample = 0.0f;

        if (noteIsActive)
        {
            // Generar la muestra según el tipo de onda seleccionado
            switch (waveType)
            {
                case 0: outputSample = generateSine     (phase); break;
                case 1: outputSample = generateSaw      (phase); break;
                case 2: outputSample = generateSquare   (phase); break;
                case 3: outputSample = generateTriangle (phase); break;
                default: outputSample = generateSine    (phase); break;
            }

            // Aplicar volumen y velocity
            outputSample *= volume * currentVelocity;

            // Avanzar la fase para el siguiente sample
            // Cuando llega a 1.0 (un ciclo completo), vuelve a 0.0
            phase += phaseIncrement;
            if (phase >= 1.0)
                phase -= 1.0;
        }

        // Escribir la misma muestra en izquierdo y derecho (mono duplicado)
        leftChannel[sample]  = outputSample;
        rightChannel[sample] = outputSample;
    }
}

//==============================================================================
// GENERADORES DE ONDA
// Cada función recibe la fase (0.0 a 1.0) y devuelve una muestra (-1.0 a 1.0)
//==============================================================================

// SENO: usa la función matemática sin()
// Fase 0.0 a 1.0 → ángulo 0 a 2π
float FractalisAudioProcessor::generateSine (double currentPhase)
{
    return (float) std::sin (currentPhase * juce::MathConstants<double>::twoPi);
}

// SIERRA (SAW): sube linealmente de -1 a +1 en un ciclo
// Fórmula directa: (fase * 2) - 1
// Ejemplo: fase=0.0 → -1.0, fase=0.5 → 0.0, fase=0.99 → ~0.98
float FractalisAudioProcessor::generateSaw (double currentPhase)
{
    return (float) (2.0 * currentPhase - 1.0);
}

// CUADRADA (SQUARE): +1 en la primera mitad del ciclo, -1 en la segunda
// Esto crea los armónicos impares característicos (1, 3, 5, 7...)
float FractalisAudioProcessor::generateSquare (double currentPhase)
{
    return currentPhase < 0.5 ? 1.0f : -1.0f;
}

// TRIANGULAR: sube de -1 a +1 en la primera mitad, baja de +1 a -1 en la segunda
// Es como una sierra pero simétrica, más suave
float FractalisAudioProcessor::generateTriangle (double currentPhase)
{
    if (currentPhase < 0.5)
        return (float) (4.0 * currentPhase - 1.0);   // sube: -1 → +1
    else
        return (float) (3.0 - 4.0 * currentPhase);   // baja: +1 → -1
}

//==============================================================================
// CONVERSIÓN MIDI → FRECUENCIA
// Fórmula estándar: A4 (nota 69) = 440Hz
// Cada semitono = multiplicar por 2^(1/12)
//==============================================================================
double FractalisAudioProcessor::midiNoteToFrequency (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

//==============================================================================
// ESTADO (PRESETS)
// Por ahora JUCE maneja esto automáticamente con el APVTS
//==============================================================================
void FractalisAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FractalisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
const juce::String FractalisAudioProcessor::getName() const { return JucePlugin_Name; }
bool FractalisAudioProcessor::acceptsMidi() const { return true; }
bool FractalisAudioProcessor::producesMidi() const { return false; }
bool FractalisAudioProcessor::isMidiEffect() const { return false; }
double FractalisAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int FractalisAudioProcessor::getNumPrograms() { return 1; }
int FractalisAudioProcessor::getCurrentProgram() { return 0; }
void FractalisAudioProcessor::setCurrentProgram (int index) {}
const juce::String FractalisAudioProcessor::getProgramName (int index) { return {}; }
void FractalisAudioProcessor::changeProgramName (int index, const juce::String& newName) {}
bool FractalisAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* FractalisAudioProcessor::createEditor()
{
    return new FractalisAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FractalisAudioProcessor();
}