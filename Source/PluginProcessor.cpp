/*
  ==============================================================================
    PluginProcessor.cpp

    Contiene toda la logica de audio: generacion de ondas, procesamiento
    de MIDI, envelopes ADSR, y el sistema de parametros.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// LAYOUT DE PARAMETROS
//
// Define los 14 parametros del sintetizador (7 por oscilador).
// Convencion de nombres: osc1/osc2 como prefijo + nombre del parametro.
//
// Tipos de parametro usados:
//   AudioParameterBool   -> valor booleano (on/off)
//   AudioParameterChoice -> lista de opciones (indice entero)
//   AudioParameterFloat  -> valor continuo con rango y paso definidos
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    FractalisAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray waveTypes { "Sine", "Saw", "Square", "Triangle" };

    // Rango de tiempo para Attack, Decay y Release: 1ms a 5 segundos.
    // El skew factor de 0.4 comprime los valores altos y expande los bajos,
    // dando mas precision en el rango de 1ms-500ms que es el mas usado.
    juce::NormalisableRange<float> timeRange    (0.001f, 5.0f,  0.001f, 0.4f);

    // El release puede ser mas largo para colas de notas (hasta 10 segundos)
    juce::NormalisableRange<float> releaseRange (0.001f, 10.0f, 0.001f, 0.4f);

    // -------------------------------------------------------------------------
    // OSC 1
    // -------------------------------------------------------------------------

    // Activar / desactivar el oscilador (true = activo por defecto)
    layout.add (std::make_unique<juce::AudioParameterBool> (
        "osc1Enabled", "OSC 1 Enable", true));

    // Tipo de onda: 0=Sine, 1=Saw, 2=Square, 3=Triangle (Saw por defecto)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "osc1WaveType", "OSC 1 Wave", waveTypes, 1));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc1Volume", "OSC 1 Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc1Attack",  "OSC 1 Attack",  timeRange,    0.01f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc1Decay",   "OSC 1 Decay",   timeRange,    0.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc1Sustain", "OSC 1 Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc1Release", "OSC 1 Release", releaseRange, 0.2f));

    // -------------------------------------------------------------------------
    // OSC 2 (desactivado por defecto para que al abrir el plugin
    // solo se escuche OSC 1 y no haya confusion inicial)
    // -------------------------------------------------------------------------

    layout.add (std::make_unique<juce::AudioParameterBool> (
        "osc2Enabled", "OSC 2 Enable", false));

    // Square por defecto para que sea distinguible de OSC 1
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        "osc2WaveType", "OSC 2 Wave", waveTypes, 2));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Volume", "OSC 2 Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Attack",  "OSC 2 Attack",  timeRange,    0.01f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Decay",   "OSC 2 Decay",   timeRange,    0.1f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Sustain", "OSC 2 Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        "osc2Release", "OSC 2 Release", releaseRange, 0.2f));

    return layout;
}

//==============================================================================
// CONSTRUCTOR
//
// Inicializa el APVTS pasandole:
//   *this             -> referencia al procesador para notificaciones
//   nullptr           -> sin UndoManager por ahora
//   "Parameters"      -> nombre del nodo raiz en el XML de estado guardado
//   createParameterLayout() -> la lista de parametros definida arriba
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
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

FractalisAudioProcessor::~FractalisAudioProcessor() {}

//==============================================================================
// PREPARE TO PLAY
//
// El DAW llama esta funcion antes de comenzar la reproduccion.
// Los objetos ADSR necesitan conocer el sampleRate para calcular correctamente
// los tiempos de ataque, decay y release en segundos.
// Si el sampleRate cambia (por ejemplo, al cambiar la configuracion del DAW),
// esta funcion se vuelve a llamar y los ADSR se actualizan.
//==============================================================================
void FractalisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    adsr1.setSampleRate (sampleRate);
    adsr2.setSampleRate (sampleRate);
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
// PROCESS BLOCK
//
// Esta es la funcion mas importante del plugin. El DAW la llama continuamente,
// miles de veces por segundo. Cada llamada entrega un bloque de muestras
// (tipicamente 64-512 samples a 44100Hz = cada 1.5ms a 11.6ms).
//
// Flujo de trabajo:
//   1. Leer todos los parametros del APVTS
//   2. Actualizar los parametros ADSR en ambos envelopes
//   3. Procesar los mensajes MIDI del bloque (noteOn, noteOff)
//   4. Generar audio muestra por muestra
//==============================================================================
void FractalisAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // -------------------------------------------------------------------------
    // 1. LEER PARAMETROS
    //
    // getRawParameterValue devuelve un puntero atomico (std::atomic<float>*).
    // Usamos .load() para leer el valor de forma thread-safe, ya que la GUI
    // puede modificar parametros desde el hilo de UI mientras el audio corre
    // en el hilo de audio.
    // -------------------------------------------------------------------------

    const bool  osc1Enabled  = apvts.getRawParameterValue ("osc1Enabled") ->load() > 0.5f;
    const int   osc1WaveType = (int) apvts.getRawParameterValue ("osc1WaveType")->load();
    const float osc1Volume   = apvts.getRawParameterValue ("osc1Volume")  ->load();

    const bool  osc2Enabled  = apvts.getRawParameterValue ("osc2Enabled") ->load() > 0.5f;
    const int   osc2WaveType = (int) apvts.getRawParameterValue ("osc2WaveType")->load();
    const float osc2Volume   = apvts.getRawParameterValue ("osc2Volume")  ->load();

    // -------------------------------------------------------------------------
    // 2. ACTUALIZAR PARAMETROS ADSR
    //
    // Los parametros ADSR pueden cambiar en cualquier momento (el usuario mueve
    // un knob). Los leemos y aplicamos una vez por bloque, no por muestra,
    // para mayor eficiencia. juce::ADSR::Parameters es una estructura simple
    // con cuatro floats: attack, decay, sustain, release.
    // -------------------------------------------------------------------------

    juce::ADSR::Parameters adsr1Params;
    adsr1Params.attack  = apvts.getRawParameterValue ("osc1Attack") ->load();
    adsr1Params.decay   = apvts.getRawParameterValue ("osc1Decay")  ->load();
    adsr1Params.sustain = apvts.getRawParameterValue ("osc1Sustain")->load();
    adsr1Params.release = apvts.getRawParameterValue ("osc1Release")->load();
    adsr1.setParameters (adsr1Params);

    juce::ADSR::Parameters adsr2Params;
    adsr2Params.attack  = apvts.getRawParameterValue ("osc2Attack") ->load();
    adsr2Params.decay   = apvts.getRawParameterValue ("osc2Decay")  ->load();
    adsr2Params.sustain = apvts.getRawParameterValue ("osc2Sustain")->load();
    adsr2Params.release = apvts.getRawParameterValue ("osc2Release")->load();
    adsr2.setParameters (adsr2Params);

    // -------------------------------------------------------------------------
    // 3. PROCESAR MENSAJES MIDI
    //
    // Iteramos por todos los eventos MIDI que llegaron en este bloque.
    // Nota: un procesamiento mas preciso colocaria cada evento en su posicion
    // exacta dentro del bloque (sample-accurate MIDI). Por ahora procesamos
    // todos los eventos antes del loop de audio, lo cual es suficiente para
    // la mayoria de los casos de uso.
    // -------------------------------------------------------------------------

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            noteIsActive     = true;
            currentFrequency = midiNoteToFrequency (message.getNoteNumber());
            currentVelocity  = message.getVelocity() / 127.0f;

            // phaseIncrement: cuanto avanza la fase por sample para generar
            // la frecuencia correcta. A 440Hz con 44100Hz de sample rate:
            // increment = 440 / 44100 = 0.00997... (un ciclo cada 100.2 samples)
            phaseIncrement = currentFrequency / getSampleRate();

            // Reiniciar fases al inicio de cada nota para evitar clicks
            // provocados por comenzar el ciclo en un punto arbitrario
            osc1Phase = 0.0;
            osc2Phase = 0.0;

            // Notificar a ambos envelopes que comienza una nota.
            // Esto dispara la etapa de Attack independientemente del estado
            // anterior del envelope.
            adsr1.noteOn();
            adsr2.noteOn();
        }
        else if (message.isNoteOff())
        {
            noteIsActive    = false;
            currentVelocity = 0.0f;

            // noteOff dispara la etapa de Release en ambos envelopes.
            // El audio sigue generandose durante el release hasta que
            // adsr.isActive() devuelve false.
            adsr1.noteOff();
            adsr2.noteOff();
        }
        else if (message.isAllNotesOff())
        {
            // Panic: cortar todo audio inmediatamente
            noteIsActive    = false;
            currentVelocity = 0.0f;
            osc1Phase       = 0.0;
            osc2Phase       = 0.0;
            adsr1.reset();
            adsr2.reset();
        }
    }

    // -------------------------------------------------------------------------
    // 4. GENERAR AUDIO MUESTRA POR MUESTRA
    //
    // Obtenemos punteros directos al buffer de salida para escritura eficiente.
    // Los buffers son arrays de float en memoria contigua.
    // -------------------------------------------------------------------------

    auto* leftChannel  = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float osc1Out = 0.0f;
        float osc2Out = 0.0f;

        // OSC 1:
        // adsr1.isActive() devuelve true durante Attack, Decay, Sustain y Release.
        // Cuando el Release termina devuelve false y el oscilador se silencia.
        // Esto permite que la nota suene aunque el usuario ya haya soltado la tecla
        // (durante la cola del release).
        if (adsr1.isActive())
        {
            // Generar la muestra de onda segun el tipo seleccionado
            const float wave1 = generateWave (osc1Phase, osc1WaveType);

            // getNextSample() avanza el envelope un step y devuelve el valor
            // actual (0.0 a 1.0). Multiplicamos la onda por este valor para
            // darle la forma del envelope.
            const float env1 = adsr1.getNextSample();

            // Solo contribuir al audio si el oscilador esta habilitado
            if (osc1Enabled)
                osc1Out = wave1 * env1 * osc1Volume * currentVelocity;

            // Avanzar la fase para el siguiente sample.
            // El modulo 1.0 mantiene la fase en el rango [0.0, 1.0).
            osc1Phase += phaseIncrement;
            if (osc1Phase >= 1.0) osc1Phase -= 1.0;
        }

        // OSC 2: mismo proceso que OSC 1 con sus propios parametros
        if (adsr2.isActive())
        {
            const float wave2 = generateWave (osc2Phase, osc2WaveType);
            const float env2  = adsr2.getNextSample();

            if (osc2Enabled)
                osc2Out = wave2 * env2 * osc2Volume * currentVelocity;

            osc2Phase += phaseIncrement;
            if (osc2Phase >= 1.0) osc2Phase -= 1.0;
        }

        // Mezclar ambos osciladores.
        // El factor 0.5 asegura que con ambos osciladores a volumen maximo
        // la salida no exceda el rango [-1.0, 1.0] y no cause clipping.
        // Con un solo oscilador activo la salida maxima sera 0.5, lo cual
        // es correcto: el usuario puede compensar con el knob de volumen.
        const float outputSample = (osc1Out + osc2Out) * 0.5f;

        // Escribir la misma muestra en ambos canales (mono duplicado a estereo).
        // En el futuro se puede agregar panorama, chorus estereo, etc.
        leftChannel [sample] = outputSample;
        rightChannel[sample] = outputSample;
    }
}

//==============================================================================
// GENERADORES DE ONDA
//
// Todos reciben la fase en rango [0.0, 1.0) donde:
//   0.0 = inicio del ciclo
//   0.5 = mitad del ciclo
//   1.0 = fin del ciclo (equivalente al inicio del siguiente)
//
// Todos devuelven valores en el rango [-1.0, 1.0].
//==============================================================================

// Dispatcher: selecciona el generador correcto segun waveType (0-3)
float FractalisAudioProcessor::generateWave (double currentPhase, int waveType)
{
    switch (waveType)
    {
        case 0:  return generateSine     (currentPhase);
        case 1:  return generateSaw      (currentPhase);
        case 2:  return generateSquare   (currentPhase);
        case 3:  return generateTriangle (currentPhase);
        default: return generateSine     (currentPhase);
    }
}

// SENO: funcion trigonometrica pura.
// Sin armonicos, sonido suave y puro. Fundamental solamente.
float FractalisAudioProcessor::generateSine (double currentPhase)
{
    return (float) std::sin (currentPhase * juce::MathConstants<double>::twoPi);
}

// SIERRA (Sawtooth): sube linealmente de -1 a +1 y cae abruptamente.
// Rica en armonicos pares e impares (1, 2, 3, 4...).
// Sonido brillante, clasico para bajos y leads.
// Nota: esta implementacion tiene aliasing. Se mejorara con PolyBLEP mas adelante.
float FractalisAudioProcessor::generateSaw (double currentPhase)
{
    return (float) (2.0 * currentPhase - 1.0);
}

// CUADRADA (Square): +1 durante la primera mitad, -1 durante la segunda.
// Contiene solo armonicos impares (1, 3, 5, 7...).
// Sonido hueco y nasal, clasico para leads y bajos con cuerpo.
float FractalisAudioProcessor::generateSquare (double currentPhase)
{
    return currentPhase < 0.5 ? 1.0f : -1.0f;
}

// TRIANGULAR (Triangle): rampa lineal simetrica.
// Solo armonicos impares como la cuadrada, pero con caida de -12dB/oct
// en lugar de -6dB/oct. Sonido mas suave y mellizo que la cuadrada.
float FractalisAudioProcessor::generateTriangle (double currentPhase)
{
    if (currentPhase < 0.5)
        return (float) (4.0 * currentPhase - 1.0);   // sube de -1 a +1
    else
        return (float) (3.0 - 4.0 * currentPhase);   // baja de +1 a -1
}

//==============================================================================
// CONVERSION MIDI A FRECUENCIA
//
// Formula de afinacion igual temperada:
//   f(n) = 440 * 2^((n - 69) / 12)
//
// La nota 69 es A4 (La central) = 440 Hz.
// Cada semitono multiplica la frecuencia por 2^(1/12) = ~1.0595.
// Cada octava dobla la frecuencia (12 semitonos = 2^1 = x2).
//==============================================================================
double FractalisAudioProcessor::midiNoteToFrequency (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

//==============================================================================
// SISTEMA DE PRESETS (ESTADO)
//
// JUCE serializa el estado completo del APVTS a un bloque de memoria XML.
// El DAW llama getStateInformation() para guardar el preset y
// setStateInformation() para restaurarlo.
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
// BOILERPLATE DE JUCE (funciones requeridas sin logica especial)
//==============================================================================
const juce::String FractalisAudioProcessor::getName() const             { return JucePlugin_Name; }
bool FractalisAudioProcessor::acceptsMidi() const                       { return true; }
bool FractalisAudioProcessor::producesMidi() const                      { return false; }
bool FractalisAudioProcessor::isMidiEffect() const                      { return false; }
double FractalisAudioProcessor::getTailLengthSeconds() const            { return 0.0; }
int FractalisAudioProcessor::getNumPrograms()                           { return 1; }
int FractalisAudioProcessor::getCurrentProgram()                        { return 0; }
void FractalisAudioProcessor::setCurrentProgram (int index)             {}
const juce::String FractalisAudioProcessor::getProgramName (int index)  { return {}; }
void FractalisAudioProcessor::changeProgramName (int index, const juce::String& newName) {}
bool FractalisAudioProcessor::hasEditor() const                         { return true; }

juce::AudioProcessorEditor* FractalisAudioProcessor::createEditor()
{
    return new FractalisAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FractalisAudioProcessor();
}