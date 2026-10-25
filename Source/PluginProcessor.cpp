#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

KozyavkaProcessor::KozyavkaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", Params::createLayout())
{
    // Запоминаем указатели на все параметры: в аудиопотоке поиск без выделения памяти
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            rawParams[std::string_view (ranged->paramID.toRawUTF8())] = apvts.getRawParameterValue (ranged->paramID);

    for (int i = 0; i < Params::numOscillators; ++i)
    {
        const char* names[] { "wave", "coarse", "fine", "pan", "level" };
        for (size_t k = 0; k < 5; ++k)
            oscRaw[(size_t) i][k] = apvts.getRawParameterValue (Params::osc (i, names[k]));
    }

    BuiltinSamples::get();   // встроенные сэмплы генерируются один раз
    engine.setVisualFeed (&visualFeed);
}

float KozyavkaProcessor::value (std::string_view id) const noexcept
{
    const auto it = rawParams.find (id);
    jassert (it != rawParams.end());
    return it != rawParams.end() ? it->second->load (std::memory_order_relaxed) : 0.0f;
}

void KozyavkaProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engine.setSettings (readSettings());
    effects.prepare (sampleRate);
    lastMaster = value (Params::master);
}

bool KozyavkaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

//==============================================================================
SynthSettings KozyavkaProcessor::readSettings() const
{
    SynthSettings s;

    for (size_t i = 0; i < s.osc.size(); ++i)
    {
        const auto& p = oscRaw[i];
        s.osc[i].wave      = (int) p[0]->load();
        s.osc[i].semitones = p[1]->load() + p[2]->load() * 0.01f;
        s.osc[i].pan       = p[3]->load();
        s.osc[i].level     = p[4]->load();
    }

    s.attack    = value (Params::attack);
    s.decay     = value (Params::decay);
    s.sustain   = value (Params::sustain);
    s.release   = value (Params::release);
    s.cutoff    = value (Params::cutoff);
    s.resonance = value (Params::resonance);
    s.filterEnv = value (Params::filterEnv);
    s.lfoRate   = value (Params::lfoRate);
    s.lfoAmount = value (Params::lfoAmount);
    s.lfoTarget = (int) value (Params::lfoTarget);
    s.glide     = value (Params::glide);
    s.maxVoices = (int) value (Params::voices);
    s.harmony   = (int) value (Params::harmony);

    auto& m = s.smp;
    m.level     = value (Params::smpLevel);
    m.semitones = value (Params::smpCoarse) + value (Params::smpFine) * 0.01f;
    m.pan       = value (Params::smpPan);
    m.mode      = (int) value (Params::smpMode);
    m.position  = value (Params::smpPosition);
    m.scan      = value (Params::smpScan);
    m.loop      = value (Params::smpLoop) > 0.5f;
    m.keytrack  = value (Params::smpKeytrack) > 0.5f;
    m.grainSize = value (Params::smpGrain);
    m.density   = value (Params::smpDensity);
    m.spray     = value (Params::smpSpray);
    m.jitter    = value (Params::smpJitter);
    m.reverse   = value (Params::smpReverse);
    m.blur      = value (Params::smpBlur);
    m.shiftHz   = Params::shiftToHz (value (Params::smpShift));
    m.sparse    = value (Params::smpSparse);
    m.whisper   = value (Params::smpWhisper);
    return s;
}

FxSettings KozyavkaProcessor::readFxSettings() const
{
    FxSettings s;
    s.textureType      = (int) value (Params::textureType);
    s.textureAmount    = value (Params::textureAmount);
    s.textureTone      = value (Params::textureTone);
    s.textureDensity   = value (Params::textureDensity);
    s.textureVariation = value (Params::textureVariation);
    s.textureWidth     = value (Params::textureWidth);
    s.textureSeed      = (int) value (Params::textureSeed);
    s.textureAlways    = value (Params::textureFollow) > 0.5f;
    s.drive            = value (Params::drive);
    s.driveMode        = (int) value (Params::driveMode);
    s.vowel            = value (Params::vowel);
    s.vowelMix         = value (Params::vowelMix);
    s.crushBits        = value (Params::crushBits);
    s.crushRate        = value (Params::crushRate);
    s.crushMix         = value (Params::crushMix);
    s.motionDepth      = value (Params::motionDepth);
    s.motionRate       = (int) value (Params::motionRate);
    s.motionShape      = (int) value (Params::motionShape);
    s.crystalMix       = value (Params::crystalMix);
    s.crystalFeedback  = value (Params::crystalFeedback);
    s.crystalPitch     = (int) value (Params::crystalPitch);
    s.spaceMix         = value (Params::spaceMix);
    s.spaceSize        = value (Params::spaceSize);
    s.spacePredelay    = value (Params::spacePredelay);
    s.shimmer          = value (Params::shimmer);
    s.spaceColor       = (int) value (Params::spaceColor);
    s.freeze           = value (Params::freeze) > 0.5f;
    return s;
}

Transport KozyavkaProcessor::readTransport()
{
    Transport t;

    if (auto* head = getPlayHead())
    {
        if (auto position = head->getPosition())
        {
            if (auto bpm = position->getBpm(); bpm.hasValue() && *bpm > 0.0)
                t.bpm = *bpm;
            if (auto ppq = position->getPpqPosition())
                t.ppq = *ppq;
            t.isPlaying = position->getIsPlaying();
        }
    }

    return t;
}

const SampleData* KozyavkaProcessor::getCurrentSample() const
{
    const int source = (int) value (Params::smpSource);
    if (source < BuiltinSamples::count)
        return BuiltinSamples::get().items[(size_t) source].get();
    return userSample.load();
}

const SampleData* KozyavkaProcessor::getTextureSample() const
{
    if (auto* s = userTexture.load())
        return s;
    return BuiltinSamples::get().items[3].get();   // без своего файла — встроенное «дыхание»
}

void KozyavkaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    if (buffer.getNumChannels() < 2)
        return;

    engine.setSample (getCurrentSample());
    engine.setSettings (readSettings());
    engine.process (buffer, midi);
    effects.process (buffer, readFxSettings(), engine.getActivity(), readTransport(), getTextureSample());

    const float newMaster = value (Params::master);
    buffer.applyGainRamp (0, buffer.getNumSamples(), lastMaster, newMaster);
    lastMaster = newMaster;

    tap.push (buffer.getReadPointer (0), buffer.getReadPointer (1), buffer.getNumSamples());
    guiActivity.store (engine.getActivity(), std::memory_order_relaxed);
    blockCounter.fetch_add (1, std::memory_order_release);
}

//==============================================================================
bool KozyavkaProcessor::loadSampleFile (const juce::File& file, Slot slot, juce::String& error)
{
    auto data = SampleData::fromFile (file, error);
    if (data == nullptr)
        return false;

    auto& target = slot == Slot::sample ? userSample : userTexture;
    const SampleData* old = target.exchange (data.get());
    ownedSamples.push_back (std::move (data));

    // Старый сэмпл откладываем «в карантин» до конца текущих аудиоблоков
    const auto now = blockCounter.load (std::memory_order_acquire);
    for (auto it = ownedSamples.begin(); it != ownedSamples.end(); ++it)
    {
        if (it->get() == old)
        {
            retired.push_back ({ std::move (*it), now });
            ownedSamples.erase (it);
            break;
        }
    }

    retired.erase (std::remove_if (retired.begin(), retired.end(),
                                   [now] (const Retired& r) { return now - r.stamp >= 2; }),
                   retired.end());

    apvts.state.setProperty (slot == Slot::sample ? "samplePath" : "texturePath", file.getFullPathName(), nullptr);

    // Переключаемся на загруженный звук
    auto select = [this] (const char* id, int index)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
    };

    if (slot == Slot::sample)
        select (Params::smpSource, BuiltinSamples::count);   // "User"
    else
        select (Params::textureType, 6);                     // "Sample"

    return true;
}

//==============================================================================
int KozyavkaProcessor::getNumPrograms()
{
    return (int) Presets::all().size() + 1;
}

const juce::String KozyavkaProcessor::getProgramName (int index)
{
    if (index <= 0 || index > (int) Presets::all().size())
        return "init";

    const auto& p = Presets::all()[(size_t) index - 1];
    return juce::String (p.category).toLowerCase() + " / " + p.name;
}

void KozyavkaProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, getNumPrograms() - 1, index);
    currentProgram = index;

    // Хост может вызвать это из аудиопотока, а менять параметры надо из потока интерфейса
    if (juce::MessageManager::existsAndIsCurrentThread())
        applyPreset (index);
    else
    {
        pendingProgram = index;
        triggerAsyncUpdate();
    }
}

void KozyavkaProcessor::applyPreset (int index)
{
    // Сначала всё в значения по умолчанию, потом — значения пресета
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            ranged->setValueNotifyingHost (ranged->getDefaultValue());

    if (index > 0 && index <= (int) Presets::all().size())
    {
        for (const auto& [id, v] : Presets::all()[(size_t) index - 1].values)
        {
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (p->convertTo0to1 (v));
            else
                jassertfalse;   // опечатка в ID параметра
        }
    }

    apvts.state.setProperty ("preset", index, nullptr);
}

//==============================================================================
juce::AudioProcessorEditor* KozyavkaProcessor::createEditor()
{
    return new KozyavkaEditor (*this);
}

void KozyavkaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KozyavkaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    currentProgram = (int) apvts.state.getProperty ("preset", 0);

    // Подгружаем свои сэмплы, если файлы всё ещё на месте
    for (auto [property, slot] : { std::pair { "samplePath", Slot::sample }, std::pair { "texturePath", Slot::texture } })
    {
        const juce::File file (apvts.state.getProperty (property).toString());
        juce::String error;
        if (file.existsAsFile())
        {
            const auto source = value (Params::smpSource);
            const auto texture = value (Params::textureType);
            loadSampleFile (file, slot, error);

            // loadSampleFile переключает источник — возвращаем то, что было сохранено
            if (auto* p = apvts.getParameter (Params::smpSource))  p->setValueNotifyingHost (p->convertTo0to1 (source));
            if (auto* p = apvts.getParameter (Params::textureType)) p->setValueNotifyingHost (p->convertTo0to1 (texture));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KozyavkaProcessor();
}
