/*
  ==============================================================================
    PluginProcessor.cpp — Fractalis Synthesizer v0.2

    Nuevos módulos respecto a v0.1:
      - SVFilter procesado después de mezclar ambos osciladores
      - LFO: genera onda periódica y la aplica al destino configurado
      - ModEnv: envelope disparado por noteOn, aplicado a su destino
      - applyModulation(): escala y clipea la modulación al rango del destino
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
// LAYOUT DE PARÁMETROS
// =============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
    FractalisAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray waveTypes { "Sine", "Saw", "Square", "Triangle" };

    // Rango de tiempo con skew 0.4 — más resolución en 1ms–500ms
    juce::NormalisableRange<float> timeRange (0.001f, 5.0f, 0.001f, 0.4f);

    // -------------------------------------------------------------------------
    // OSC 1 (idéntico a v0.1)
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterBool>   ("osc1Enabled",  "OSC 1 Enable", true));
    layout.add (std::make_unique<juce::AudioParameterChoice> ("osc1WaveType", "OSC 1 Wave",  waveTypes, 1));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc1Volume",   "OSC 1 Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc1Attack",   "OSC 1 Attack",  timeRange, 0.01f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc1Decay",    "OSC 1 Decay",   timeRange, 0.1f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc1Sustain",  "OSC 1 Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc1Release",  "OSC 1 Release", timeRange, 0.2f));

    // -------------------------------------------------------------------------
    // OSC 2 — desactivado por defecto, Square para distinguirlo de OSC 1
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterBool>   ("osc2Enabled",  "OSC 2 Enable", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> ("osc2WaveType", "OSC 2 Wave",  waveTypes, 2));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc2Volume",   "OSC 2 Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc2Attack",   "OSC 2 Attack",  timeRange, 0.01f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc2Decay",    "OSC 2 Decay",   timeRange, 0.1f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc2Sustain",  "OSC 2 Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));
    layout.add (std::make_unique<juce::AudioParameterFloat>  ("osc2Release",  "OSC 2 Release", timeRange, 0.2f));

    // -------------------------------------------------------------------------
    // FILTRO — Cutoff logarítmico 20–20000 Hz, resonance 0.1–10
    // -------------------------------------------------------------------------
    // skew 0.3 en cutoff comprime los valores altos y expande los bajos,
    // reproduciendo la percepción logarítmica del oído humano en el knob.
    layout.add (std::make_unique<juce::AudioParameterFloat> ("filterCutoff", "Filter Cutoff",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 8000.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("filterRes", "Filter Resonance",
        juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 0.7f));
    // Modos: 0=LP, 1=HP, 2=BP, 3=Notch
    layout.add (std::make_unique<juce::AudioParameterChoice> ("filterMode", "Filter Mode",
        juce::StringArray { "LP", "HP", "BP", "Notch" }, 0));

    // -------------------------------------------------------------------------
    // LFO — Rate (0.01–20 Hz), Depth (0–1), forma de onda, destino
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> ("lfoRate", "LFO Rate",
        juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.4f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("lfoDepth", "LFO Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> ("lfoWave", "LFO Wave",
        waveTypes, 0));
    // Destino: índice de ModDestination (0 = None)
    layout.add (std::make_unique<juce::AudioParameterChoice> ("lfoDest", "LFO Dest",
        kModDestNames, 0));

    // -------------------------------------------------------------------------
    // MOD ENV — ADSR independiente + Amount (−1 a +1) + destino
    // -------------------------------------------------------------------------
    layout.add (std::make_unique<juce::AudioParameterFloat> ("modEnvAttack",  "ModEnv Attack",  timeRange, 0.05f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("modEnvDecay",   "ModEnv Decay",   timeRange, 0.3f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("modEnvSustain", "ModEnv Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("modEnvRelease", "ModEnv Release", timeRange, 0.1f));
    // Amount bipolar: −1 invierte la modulación, +1 al máximo
    layout.add (std::make_unique<juce::AudioParameterFloat> ("modEnvAmount", "ModEnv Amount",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> ("modEnvDest", "ModEnv Dest",
        kModDestNames, 0));

    return layout;
}

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================
FractalisAudioProcessor::FractalisAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

FractalisAudioProcessor::~FractalisAudioProcessor() {}

// =============================================================================
// PREPARE TO PLAY — inicializa módulos con el sample rate del DAW
// =============================================================================
void FractalisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& v : voices)
    {
        v.adsr1.setSampleRate (sampleRate);
        v.adsr2.setSampleRate (sampleRate);
        v.reset();
    }
    modEnv.setSampleRate (sampleRate);
    modEnv.reset();
    filter.setSampleRate (sampleRate);
    filter.reset();
    lfoPhase = 0.0;
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
    return true;
  #endif
}
#endif

// =============================================================================
// APPLY MODULATION
//
// Recibe el destino, el valor base del parámetro y el valor modulador
// normalizado (−1…+1 para LFO, 0…+1 para ModEnv escalado por Amount).
// Retorna el valor final clampeado al rango válido del destino.
//
// Para parámetros de tiempo (ATK/DCY/REL), la modulación se aplica
// multiplicativamente para evitar valores negativos: base * (1 + modValue).
// Para volúmenes, cutoff y resonance se aplica aditivamente.
// =============================================================================
float FractalisAudioProcessor::applyModulation (ModDestination dest,
                                                float baseValue,
                                                float modValue)
{
    float out = baseValue;

    switch (dest)
    {
        // Volúmenes: rango 0–1
        case ModDestination::Osc1Volume:
        case ModDestination::Osc2Volume:
            out = juce::jlimit (0.0f, 1.0f, baseValue + modValue);
            break;

        // Tiempos de envelope: modulación multiplicativa (rango 0.001–5 s)
        case ModDestination::Osc1Attack:  case ModDestination::Osc2Attack:
        case ModDestination::Osc1Decay:   case ModDestination::Osc2Decay:
        case ModDestination::Osc1Release: case ModDestination::Osc2Release:
            out = juce::jlimit (0.001f, 5.0f, baseValue * (1.0f + modValue));
            break;

        // Sustain: rango 0–1
        case ModDestination::Osc1Sustain:
        case ModDestination::Osc2Sustain:
            out = juce::jlimit (0.0f, 1.0f, baseValue + modValue);
            break;

        // Cutoff: rango 20–20000 Hz, modulación en Hz (modValue escala 0–10000 Hz)
        case ModDestination::FilterCutoff:
            out = juce::jlimit (20.0f, 20000.0f, baseValue + modValue * 10000.0f);
            break;

        // Resonance: rango 0.1–10
        case ModDestination::FilterResonance:
            out = juce::jlimit (0.1f, 10.0f, baseValue + modValue * 5.0f);
            break;

        default:
            out = baseValue;
            break;
    }

    return out;
}

// Devuelve una voz libre, o roba la más silenciosa si todas están ocupadas.
SynthVoice* FractalisAudioProcessor::findFreeVoice() noexcept
{
    // 1. Buscar voz completamente inactiva
    for (auto& v : voices)
        if (!v.isActive()) return &v;

    // 2. Voice stealing: la voz con el envelope más bajo (menos audible)
    SynthVoice* steal = &voices[0];
    float lowest = voices[0].adsr1.getValue();
    for (auto& v : voices)
    {
        const float val = v.adsr1.getValue();
        if (val < lowest) { lowest = val; steal = &v; }
    }
    steal->reset();
    return steal;
}

void FractalisAudioProcessor::addGuiMidiMessage (const juce::MidiMessage& msg)
{
    juce::ScopedLock lock (guiMidiLock);
    guiMidiQueue.add (msg);
}

// =============================================================================
// PROCESS BLOCK
//
// Flujo por bloque:
//   1. Leer parámetros del APVTS
//   2. Calcular valor actual del LFO (una vez por bloque, no por muestra)
//   3. Actualizar ModEnv y obtener su valor actual
//   4. Aplicar modulación a los parámetros destino
//   5. Configurar ADSR y filtro con los valores modulados
//   6. Procesar mensajes MIDI
//   7. Generar audio muestra por muestra a través del filtro
// =============================================================================
void FractalisAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Volcar eventos MIDI generados por la GUI (teclado piano clickeable)
    {
        juce::ScopedLock lock (guiMidiLock);
        for (const auto& msg : guiMidiQueue)
            midiMessages.addEvent (msg, 0);
        guiMidiQueue.clear();
    }

    // -------------------------------------------------------------------------
    // 1. LEER PARÁMETROS BASE
    // -------------------------------------------------------------------------
    const bool  osc1Enabled  = apvts.getRawParameterValue ("osc1Enabled") ->load() > 0.5f;
    const int   osc1WaveType = (int) apvts.getRawParameterValue ("osc1WaveType")->load();
    float       osc1Volume   = apvts.getRawParameterValue ("osc1Volume")  ->load();

    const bool  osc2Enabled  = apvts.getRawParameterValue ("osc2Enabled") ->load() > 0.5f;
    const int   osc2WaveType = (int) apvts.getRawParameterValue ("osc2WaveType")->load();
    float       osc2Volume   = apvts.getRawParameterValue ("osc2Volume")  ->load();

    float filterCutoff = apvts.getRawParameterValue ("filterCutoff")->load();
    float filterRes    = apvts.getRawParameterValue ("filterRes")   ->load();
    const int filterMode = (int) apvts.getRawParameterValue ("filterMode")->load();

    // Parámetros ADSR OSC 1
    juce::ADSR::Parameters adsr1Base;
    adsr1Base.attack  = apvts.getRawParameterValue ("osc1Attack") ->load();
    adsr1Base.decay   = apvts.getRawParameterValue ("osc1Decay")  ->load();
    adsr1Base.sustain = apvts.getRawParameterValue ("osc1Sustain")->load();
    adsr1Base.release = apvts.getRawParameterValue ("osc1Release")->load();

    // Parámetros ADSR OSC 2
    juce::ADSR::Parameters adsr2Base;
    adsr2Base.attack  = apvts.getRawParameterValue ("osc2Attack") ->load();
    adsr2Base.decay   = apvts.getRawParameterValue ("osc2Decay")  ->load();
    adsr2Base.sustain = apvts.getRawParameterValue ("osc2Sustain")->load();
    adsr2Base.release = apvts.getRawParameterValue ("osc2Release")->load();

    // -------------------------------------------------------------------------
    // 2. CALCULAR VALOR DEL LFO (una muestra representativa del bloque)
    //
    // Calculamos la fase al principio del bloque para usarla como modulador
    // constante durante todo el bloque.  Es una aproximación válida porque
    // los bloques son cortos (~3 ms) y el LFO cambia lentamente.
    // -------------------------------------------------------------------------
    const float lfoRate  = apvts.getRawParameterValue ("lfoRate") ->load();
    const float lfoDepth = apvts.getRawParameterValue ("lfoDepth")->load();
    const int   lfoWave  = (int) apvts.getRawParameterValue ("lfoWave")->load();
    const auto  lfoDest  = static_cast<ModDestination> (
                               (int) apvts.getRawParameterValue ("lfoDest")->load());

    // Avanzar la fase del LFO por el tamaño del bloque
    const double lfoPhaseInc = lfoRate / getSampleRate();
    lfoPhase += lfoPhaseInc * buffer.getNumSamples();
    if (lfoPhase >= 1.0) lfoPhase -= 1.0;

    // Valor LFO en rango −1…+1, escalado por depth
    const float lfoRaw   = generateWave (lfoPhase, lfoWave); // ya en −1…+1
    const float lfoValue = lfoRaw * lfoDepth;

    // -------------------------------------------------------------------------
    // 3. CALCULAR VALOR DEL MOD ENV
    //
    // ModEnv es un FixedADSR que se dispara con noteOn igual que los ADSR de
    // audio, pero su salida va al sistema de modulación, no directamente al
    // audio.  getNextSample() devuelve un valor en 0…1 que escalamos con Amount.
    // -------------------------------------------------------------------------
    const float modEnvAmount = apvts.getRawParameterValue ("modEnvAmount")->load();
    const auto  modEnvDest   = static_cast<ModDestination> (
                                   (int) apvts.getRawParameterValue ("modEnvDest")->load());

    // Actualizar parámetros ADSR del ModEnv
    juce::ADSR::Parameters modEnvParams;
    modEnvParams.attack  = apvts.getRawParameterValue ("modEnvAttack") ->load();
    modEnvParams.decay   = apvts.getRawParameterValue ("modEnvDecay")  ->load();
    modEnvParams.sustain = apvts.getRawParameterValue ("modEnvSustain")->load();
    modEnvParams.release = apvts.getRawParameterValue ("modEnvRelease")->load();
    modEnv.setParameters (modEnvParams);

    // Avanzar ModEnv una muestra (representativa del bloque)
    const float modEnvRaw   = modEnv.isActive() ? modEnv.getNextSample() : 0.0f;
    const float modEnvValue = modEnvRaw * modEnvAmount;  // bipolar si Amount < 0

    // -------------------------------------------------------------------------
    // 4. APLICAR MODULACIÓN A LOS PARÁMETROS DESTINO
    //
    // Cada modulador se aplica solo si su destino no es None.
    // Si ambos moduladores apuntan al mismo destino, sus efectos se suman.
    // -------------------------------------------------------------------------
    auto applyMod = [&](ModDestination dest, float modVal)
    {
        if (dest == ModDestination::None) return;

        switch (dest)
        {
            case ModDestination::Osc1Volume:   osc1Volume   = applyModulation (dest, osc1Volume,        modVal); break;
            case ModDestination::Osc2Volume:   osc2Volume   = applyModulation (dest, osc2Volume,        modVal); break;
            case ModDestination::Osc1Attack:   adsr1Base.attack  = applyModulation (dest, adsr1Base.attack,  modVal); break;
            case ModDestination::Osc1Decay:    adsr1Base.decay   = applyModulation (dest, adsr1Base.decay,   modVal); break;
            case ModDestination::Osc1Sustain:  adsr1Base.sustain = applyModulation (dest, adsr1Base.sustain, modVal); break;
            case ModDestination::Osc1Release:  adsr1Base.release = applyModulation (dest, adsr1Base.release, modVal); break;
            case ModDestination::Osc2Attack:   adsr2Base.attack  = applyModulation (dest, adsr2Base.attack,  modVal); break;
            case ModDestination::Osc2Decay:    adsr2Base.decay   = applyModulation (dest, adsr2Base.decay,   modVal); break;
            case ModDestination::Osc2Sustain:  adsr2Base.sustain = applyModulation (dest, adsr2Base.sustain, modVal); break;
            case ModDestination::Osc2Release:  adsr2Base.release = applyModulation (dest, adsr2Base.release, modVal); break;
            case ModDestination::FilterCutoff: filterCutoff = applyModulation (dest, filterCutoff, modVal); break;
            case ModDestination::FilterResonance: filterRes = applyModulation (dest, filterRes,    modVal); break;
            default: break;
        }
    };

    applyMod (lfoDest,    lfoValue);
    applyMod (modEnvDest, modEnvValue);

    // -------------------------------------------------------------------------
    // 5. CONFIGURAR ADSR DE TODAS LAS VOCES ACTIVAS Y FILTRO
    // -------------------------------------------------------------------------
    for (auto& v : voices)
    {
        if (v.isActive())
        {
            v.adsr1.setParameters (adsr1Base);
            v.adsr2.setParameters (adsr2Base);
        }
    }
    filter.setMode (filterMode);
    filter.setParameters (filterCutoff, filterRes);

    // -------------------------------------------------------------------------
    // 6. PROCESAR MENSAJES MIDI
    // -------------------------------------------------------------------------
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            SynthVoice* voice = findFreeVoice();
            voice->midiNote  = message.getNoteNumber();
            voice->frequency = midiNoteToFrequency (message.getNoteNumber());
            voice->phaseInc  = voice->frequency / getSampleRate();
            voice->osc1Phase = 0.0;
            voice->osc2Phase = 0.0;
            voice->adsr1.setParameters (adsr1Base);
            voice->adsr2.setParameters (adsr2Base);
            voice->adsr1.noteOn();
            voice->adsr2.noteOn();
            modEnv.noteOn();   // ModEnv global: retrigger en cada nota
            lastMidiNote.store (message.getNoteNumber());
        }
        else if (message.isNoteOff())
        {
            const int note = message.getNoteNumber();
            for (auto& v : voices)
            {
                if (v.midiNote == note)
                {
                    v.adsr1.noteOff();
                    v.adsr2.noteOff();
                    v.midiNote = -1;
                    break;  // Solo libera la primera voz con esa nota
                }
            }
            // Liberar modEnv solo si no queda ninguna tecla presionada
            bool anyHeld = false;
            for (const auto& v : voices)
                if (v.midiNote != -1) { anyHeld = true; break; }
            if (!anyHeld)
            {
                modEnv.noteOff();
                lastMidiNote.store (-1);
            }
        }
        else if (message.isAllNotesOff())
        {
            for (auto& v : voices) v.reset();
            modEnv.reset();
            filter.reset();
            lastMidiNote.store (-1);
        }
    }

    // -------------------------------------------------------------------------
    // 7. GENERAR AUDIO — suma de voces → filtro → salida
    // -------------------------------------------------------------------------
    auto* leftChannel  = buffer.getWritePointer (0);
    auto* rightChannel = buffer.getWritePointer (1);
    const int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mixed = 0.0f;

        for (auto& voice : voices)
        {
            if (!voice.isActive()) continue;

            float osc1Out = 0.0f, osc2Out = 0.0f;

            // OSC 1
            {
                const float env1  = voice.adsr1.getNextSample();
                const float wave1 = generateWave (voice.osc1Phase, osc1WaveType);
                voice.osc1Phase += voice.phaseInc;
                if (voice.osc1Phase >= 1.0) voice.osc1Phase -= 1.0;
                if (osc1Enabled) osc1Out = wave1 * env1 * osc1Volume;
            }

            // OSC 2
            if (voice.adsr2.isActive())
            {
                const float env2  = voice.adsr2.getNextSample();
                const float wave2 = generateWave (voice.osc2Phase, osc2WaveType);
                voice.osc2Phase += voice.phaseInc;
                if (voice.osc2Phase >= 1.0) voice.osc2Phase -= 1.0;
                if (osc2Enabled) osc2Out = wave2 * env2 * osc2Volume;
            }

            mixed += (osc1Out + osc2Out) * 0.5f;
        }

        // Normalizar por número máximo de voces — evita clipping con acordes
        mixed *= (1.0f / static_cast<float> (kMaxVoices));

        mixed = filter.processSample (mixed);

        leftChannel [sample] = mixed;
        rightChannel[sample] = mixed;
    }

    // Actualizar bitmask para que la GUI refleje qué notas están presionadas
    uint64_t bitsLow = 0, bitsHigh = 0;
    for (const auto& v : voices)
    {
        const int n = v.midiNote;
        if      (n >= 0  && n < 64)  bitsLow  |= (1ULL << n);
        else if (n >= 64 && n < 128) bitsHigh |= (1ULL << (n - 64));
    }
    activeNoteBitsLow.store  (bitsLow);
    activeNoteBitsHigh.store (bitsHigh);
}

// =============================================================================
// GENERADORES DE ONDA — fase en [0.0, 1.0), salida en [−1.0, 1.0]
// =============================================================================
float FractalisAudioProcessor::generateWave (double phase, int waveType)
{
    switch (waveType)
    {
        case 0:  return generateSine     (phase);
        case 1:  return generateSaw      (phase);
        case 2:  return generateSquare   (phase);
        case 3:  return generateTriangle (phase);
        default: return generateSine     (phase);
    }
}

// Seno: sin armónicos, sonido puro
float FractalisAudioProcessor::generateSine (double phase)
{
    return (float) std::sin (phase * juce::MathConstants<double>::twoPi);
}

// Sierra: armónicos pares e impares (1,2,3,4...), brillante
float FractalisAudioProcessor::generateSaw (double phase)
{
    return (float) (2.0 * phase - 1.0);
}

// Cuadrada: solo armónicos impares (1,3,5...), hueco y nasal
float FractalisAudioProcessor::generateSquare (double phase)
{
    return phase < 0.5 ? 1.0f : -1.0f;
}

// Triangular: solo impares, caída −12dB/oct, más suave que cuadrada
float FractalisAudioProcessor::generateTriangle (double phase)
{
    if (phase < 0.5) return (float) (4.0 * phase - 1.0);
    else             return (float) (3.0 - 4.0 * phase);
}

// f(n) = 440 · 2^((n−69)/12) — temperamento igual, A4=440 Hz
double FractalisAudioProcessor::midiNoteToFrequency (int midiNote)
{
    return 440.0 * std::pow (2.0, (midiNote - 69) / 12.0);
}

// =============================================================================
// ESTADO (presets) — serialización XML del APVTS
// =============================================================================
void FractalisAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void FractalisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

// =============================================================================
// BOILERPLATE JUCE
// =============================================================================
const juce::String FractalisAudioProcessor::getName() const             { return JucePlugin_Name; }
bool   FractalisAudioProcessor::acceptsMidi()  const                    { return true; }
bool   FractalisAudioProcessor::producesMidi() const                    { return false; }
bool   FractalisAudioProcessor::isMidiEffect() const                    { return false; }
double FractalisAudioProcessor::getTailLengthSeconds() const            { return 0.0; }
int    FractalisAudioProcessor::getNumPrograms()                        { return 1; }
int    FractalisAudioProcessor::getCurrentProgram()                     { return 0; }
void   FractalisAudioProcessor::setCurrentProgram (int)                 {}
const juce::String FractalisAudioProcessor::getProgramName (int)        { return {}; }
void   FractalisAudioProcessor::changeProgramName (int, const juce::String&) {}
bool   FractalisAudioProcessor::hasEditor() const                       { return true; }
juce::AudioProcessorEditor* FractalisAudioProcessor::createEditor()
{
    return new FractalisAudioProcessorEditor (*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FractalisAudioProcessor();
}