#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SampleVoice.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Настройки, которые процессор передаёт движку один раз за аудиоблок
struct SynthSettings
{
    struct Osc
    {
        int wave = 0;            // 0 Sine, 1 Triangle, 2 Saw, 3 Square, 4 Noise
        float semitones = 0.0f;  // coarse + fine
        float pan = 0.0f;
        float level = 0.0f;
    };

    std::array<Osc, 3> osc {};
    float attack = 0.005f, decay = 0.3f, sustain = 0.7f, release = 0.3f;
    float cutoff = 20000.0f, resonance = 0.0f, filterEnv = 0.0f;
    float lfoRate = 5.0f, lfoAmount = 0.0f;
    int lfoTarget = 0;           // 0 Pitch, 1 Filter, 2 Volume
    float glide = 0.0f;
    int maxVoices = 8;
    int harmony = 0;             // индекс в Params::harmonyNames
    SampleSettings smp;          // слой сэмплера
};

//==============================================================================
// Коэффициенты огибающей считаются один раз за блок и общие для всех голосов
struct EnvCoefs
{
    float attackInc = 0.01f, decayCoef = 0.999f, sustain = 0.7f, releaseCoef = 0.999f;
};

// ADSR: линейная атака, экспоненциальные спад и затухание (дёшево и звучит естественно)
class Envelope
{
public:
    void noteOn() noexcept               { stage = Stage::Attack; }   // уровень не сбрасываем, поэтому без щелчков
    void noteOff() noexcept              { if (stage != Stage::Idle) stage = Stage::Release; }
    void reset() noexcept                { stage = Stage::Idle; level = 0.0f; }
    bool isActive() const noexcept      { return stage != Stage::Idle; }
    float getLevel() const noexcept     { return level; }

    float next (const EnvCoefs& c) noexcept
    {
        switch (stage)
        {
            case Stage::Attack:
                level += c.attackInc;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;

            case Stage::Decay:
                level = c.sustain + (level - c.sustain) * c.decayCoef;
                if (std::abs (level - c.sustain) < 1.0e-4f) { level = c.sustain; stage = Stage::Sustain; }
                break;

            case Stage::Sustain:
                level = c.sustain;   // реагирует на поворот ручки Sustain
                break;

            case Stage::Release:
                level *= c.releaseCoef;
                if (level < 1.0e-4f) reset();
                break;

            case Stage::Idle:
                break;
        }

        return level;
    }

private:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Stage::Idle;
    float level = 0.0f;
};

//==============================================================================
// Резонансный ФНЧ (TPT state variable filter), стерео с общими коэффициентами
class StereoLowpass
{
public:
    void reset() noexcept { ic1 = { 0.0f, 0.0f }; ic2 = { 0.0f, 0.0f }; }

    void setCutoff (float hz, float k, float sampleRate) noexcept
    {
        hz = juce::jlimit (20.0f, sampleRate * 0.45f, hz);
        const float g = std::tan (juce::MathConstants<float>::pi * hz / sampleRate);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void process (float& left, float& right) noexcept
    {
        left  = tick (left, 0);
        right = tick (right, 1);
    }

private:
    float tick (float x, size_t ch) noexcept
    {
        const float v3 = x - ic2[ch];
        const float v1 = a1 * ic1[ch] + a2 * v3;
        const float v2 = ic2[ch] + a2 * ic1[ch] + a3 * v3;
        ic1[ch] = 2.0f * v1 - ic1[ch];
        ic2[ch] = 2.0f * v2 - ic2[ch];
        return v2;
    }

    float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f;
    std::array<float, 2> ic1 {}, ic2 {};
};

//==============================================================================
class SynthEngine
{
public:
    static constexpr int maxPolyphony = 16;
    static constexpr int controlBlock = 32;   // модуляция пересчитывается раз в 32 сэмпла

    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        invSampleRate = 1.0f / sampleRate;
        sine = sineTable();
        fft = std::make_unique<juce::dsp::FFT> (SampleData::fftOrder);
        reset();

        uint32_t index = 0;
        for (auto& v : voices)
            v.sampler.prepare (newSampleRate, fft.get(), ++index * 7919u);
    }

    void reset()
    {
        uint32_t seed = 0x9e3779b9u;
        for (auto& v : voices)
        {
            v.note = v.key = -1;
            v.gate = v.sustained = false;
            v.phase = {};
            v.env.reset();
            v.filter.reset();
            v.noise = (seed += 0x6d2b79f5u);
        }
        lfoPhase = 0.0f;
        pitchBend = 0.0f;
        sustainPedal = false;
        lastKey = -1;
        activity = 0.0f;
    }

    void setSample (const SampleData* data) noexcept          { sample = data; }
    void setVisualFeed (SampleVisualFeed* newFeed) noexcept    { feed = newFeed; }

    // Насколько громко сейчас звучат ноты (0..1): по нему текстура «дышит» вместе с синтом
    float getActivity() const noexcept { return activity; }

    void setSettings (const SynthSettings& s)
    {
        // Если полифонию уменьшили, лишние голоса плавно затухают
        for (int i = s.maxVoices; i < maxPolyphony; ++i)
            voices[(size_t) i].release();

        settings = s;

        envCoefs.attackInc   = 1.0f / (std::max (s.attack, 0.001f) * sampleRate);
        envCoefs.decayCoef   = timeToCoef (s.decay);
        envCoefs.sustain     = s.sustain;
        envCoefs.releaseCoef = timeToCoef (s.release);
        filterK = 2.0f - 1.96f * s.resonance;

        // Фильтр полностью открыт и ничем не модулируется — не тратим на него процессор
        const bool needFilter = s.cutoff < 19500.0f || std::abs (s.filterEnv) > 0.001f
                             || (s.lfoTarget == 1 && s.lfoAmount > 0.0f);
        if (needFilter && ! filterOn)
            for (auto& v : voices)
                v.filter.reset();
        filterOn = needFilter;
    }

    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
    {
        const int numSamples = buffer.getNumSamples();
        float* left  = buffer.getWritePointer (0);
        float* right = buffer.getWritePointer (1);

        // Рендерим кусками между MIDI-событиями — ноты звучат точно в такт
        int pos = 0;
        for (const auto meta : midi)
        {
            const int eventPos = juce::jlimit (0, numSamples, meta.samplePosition);
            if (eventPos > pos)
            {
                render (left + pos, right + pos, eventPos - pos);
                pos = eventPos;
            }
            handleMidi (meta.getMessage());
        }

        if (pos < numSamples)
            render (left + pos, right + pos, numSamples - pos);

        activity = activityPeak;
        activityPeak = 0.0f;
    }

private:
    struct Voice
    {
        int note = -1;            // звучащая нота
        int key = -1;             // клавиша, которая её запустила (у нот гармонии она общая)
        bool gate = false;        // клавиша нажата
        bool sustained = false;   // клавиша отпущена, но держит педаль
        float velocity = 0.0f;
        uint32_t age = 0;
        float pitch = 60.0f, targetPitch = 60.0f;
        std::array<float, 3> phase {};
        uint32_t noise = 1;
        Envelope env;
        StereoLowpass filter;
        SampleVoice sampler;

        bool isActive() const noexcept { return env.isActive(); }

        void release() noexcept
        {
            gate = sustained = false;
            env.noteOff();
        }

        float nextNoise() noexcept   // xorshift32: быстрый белый шум
        {
            noise ^= noise << 13;
            noise ^= noise >> 17;
            noise ^= noise << 5;
            return (float) (int32_t) noise * (1.0f / 2147483648.0f);
        }
    };

    struct ControlState
    {
        float pitchMod = 0.0f, filterMod = 0.0f, volumeMod = 1.0f, glideStep = 1.0f;
    };

    static constexpr int sineTableSize = 1024;

    static const float* sineTable()
    {
        static const auto table = []
        {
            std::array<float, sineTableSize + 1> t {};
            for (int i = 0; i <= sineTableSize; ++i)
                t[(size_t) i] = (float) std::sin (juce::MathConstants<double>::twoPi * i / sineTableSize);
            return t;
        }();
        return table.data();
    }

    float sineAt (float phase) const noexcept
    {
        const float x = phase * (float) sineTableSize;
        const int i = (int) x;
        return sine[i] + (x - (float) i) * (sine[i + 1] - sine[i]);
    }

    // PolyBLEP сглаживает скачки пилы и квадрата: меньше алиасинга почти бесплатно
    static float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt;          return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    float timeToCoef (float seconds) const noexcept
    {
        // за указанное время сигнал падает на 60 дБ
        return std::exp (-6.9f / (std::max (seconds, 0.001f) * sampleRate));
    }

    //==============================================================================
    void handleMidi (const juce::MidiMessage& m)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())
            pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;   // ±2 полутона
        else if (m.isSustainPedalOn())
            sustainPedal = true;
        else if (m.isSustainPedalOff())
        {
            sustainPedal = false;
            for (auto& v : voices)
                if (v.sustained)
                    v.release();
        }
        else if (m.isAllSoundOff())
        {
            for (auto& v : voices)
                v.env.reset();
        }
        else if (m.isAllNotesOff())
        {
            for (auto& v : voices)
                v.release();
        }
    }

    // Интервалы гармонизатора в полутонах
    static const std::vector<int>& harmonyIntervals (int index)
    {
        static const std::array<std::vector<int>, 9> table {{
            {}, { 12 }, { 7 }, { 7, 12 }, { 4, 7 }, { 3, 7 }, { 2, 7 }, { 3, 7, 10 }, { -12 }
        }};
        return table[(size_t) juce::jlimit (0, (int) table.size() - 1, index)];
    }

    void noteOn (int key, float velocity)
    {
        startVoice (key, key, velocity);

        for (int interval : harmonyIntervals (settings.harmony))
            if (juce::isPositiveAndBelow (key + interval, 128))
                startVoice (key, key + interval, velocity * 0.75f);

        lastKey = key;
    }

    void startVoice (int key, int note, float velocity)
    {
        const size_t limit = (size_t) juce::jlimit (1, maxPolyphony, settings.maxVoices);
        Voice* target = nullptr;

        // 1) эта же нота уже звучит — перезапускаем её
        for (size_t i = 0; i < limit && target == nullptr; ++i)
            if (voices[i].note == note && voices[i].isActive())
                target = &voices[i];

        // 2) свободный голос
        for (size_t i = 0; i < limit && target == nullptr; ++i)
            if (! voices[i].isActive())
                target = &voices[i];

        // 3) все заняты — забираем самый старый, сначала среди отпущенных клавиш
        if (target == nullptr)
        {
            for (size_t i = 0; i < limit; ++i)
            {
                auto& v = voices[i];
                if (target == nullptr
                    || (target->gate && ! v.gate)
                    || (target->gate == v.gate && v.age < target->age))
                    target = &v;
            }
        }

        auto& v = *target;
        const bool wasActive = v.isActive();

        if (! wasActive)
        {
            v.phase = {};
            v.filter.reset();
        }

        // Глайд: каждая нота аккорда скользит от того же интервала предыдущей клавиши
        const bool useGlide = settings.glide > 0.0005f && lastKey >= 0;
        const float glideFrom = (float) (lastKey + note - key);
        v.pitch = useGlide ? (wasActive ? v.pitch : glideFrom) : (float) note;
        v.targetPitch = (float) note;
        v.note = note;
        v.key = key;
        v.velocity = velocity;
        v.gate = true;
        v.sustained = false;
        v.age = ++noteCounter;
        v.env.noteOn();
        v.sampler.start (sample, settings.smp);
    }

    void noteOff (int key)
    {
        for (auto& v : voices)
        {
            if (v.gate && v.key == key)
            {
                v.gate = false;
                if (sustainPedal)
                    v.sustained = true;
                else
                    v.env.noteOff();
            }
        }
    }

    //==============================================================================
    void render (float* left, float* right, int numSamples)
    {
        while (numSamples > 0)
        {
            const int n = std::min (numSamples, controlBlock);
            const auto cs = updateControl (n);

            for (auto& v : voices)
                if (v.isActive())
                    renderVoice (v, left, right, n, cs);

            left += n;
            right += n;
            numSamples -= n;
        }
    }

    ControlState updateControl (int n)
    {
        ControlState cs;
        cs.pitchMod = pitchBend;

        if (settings.lfoAmount > 0.0f)
        {
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * lfoPhase);

            switch (settings.lfoTarget)
            {
                case 0:  cs.pitchMod += lfo * settings.lfoAmount * 2.0f; break;                      // вибрато до ±2 полутонов
                case 1:  cs.filterMod = lfo * settings.lfoAmount * 3.0f; break;                      // до ±3 октав
                default: cs.volumeMod = 1.0f - settings.lfoAmount * 0.5f * (1.0f + lfo); break;     // тремоло
            }
        }

        lfoPhase += settings.lfoRate * (float) n * invSampleRate;
        lfoPhase -= std::floor (lfoPhase);

        if (settings.glide > 0.0005f)
            cs.glideStep = 1.0f - std::exp (-(float) n / (settings.glide * sampleRate));

        return cs;
    }

    void renderVoice (Voice& v, float* outL, float* outR, int n, const ControlState& cs)
    {
        float bufL[controlBlock] = {};
        float bufR[controlBlock] = {};

        v.pitch += (v.targetPitch - v.pitch) * cs.glideStep;

        for (size_t o = 0; o < 3; ++o)
        {
            const auto& osc = settings.osc[o];
            if (osc.level <= 0.0f)
                continue;   // выключенный осциллятор ничего не стоит

            const float gainL = osc.level * std::min (1.0f, 1.0f - osc.pan);
            const float gainR = osc.level * std::min (1.0f, 1.0f + osc.pan);
            const float freq  = 440.0f * std::exp2 ((v.pitch + osc.semitones + cs.pitchMod - 69.0f) / 12.0f);
            const float inc   = std::min (freq * invSampleRate, 0.45f);
            float phase = v.phase[o];

            auto run = [&] (auto&& generate)
            {
                for (int i = 0; i < n; ++i)
                {
                    const float s = generate (phase);
                    bufL[i] += s * gainL;
                    bufR[i] += s * gainR;
                    phase += inc;
                    if (phase >= 1.0f)
                        phase -= 1.0f;
                }
            };

            switch (osc.wave)
            {
                case 0:  run ([this] (float p) { return sineAt (p); }); break;
                case 1:  run ([] (float p) { return 4.0f * std::abs (p - 0.5f) - 1.0f; }); break;
                case 2:  run ([inc] (float p) { return 2.0f * p - 1.0f - polyBlep (p, inc); }); break;
                case 3:  run ([inc] (float p)
                         {
                             float p2 = p + 0.5f;
                             if (p2 >= 1.0f) p2 -= 1.0f;
                             return (p < 0.5f ? 1.0f : -1.0f) + polyBlep (p, inc) - polyBlep (p2, inc);
                         });
                         break;
                default: run ([&v] (float) { return v.nextNoise(); }); break;
            }

            v.phase[o] = phase;
        }

        // Слой сэмплера идёт через ту же огибающую и фильтр, что и осцилляторы
        if (sample != nullptr && settings.smp.level > 0.0f)
        {
            const float basePitch = settings.smp.keytrack ? v.pitch : (float) sample->rootNote;
            v.sampler.render (*sample, settings.smp, basePitch + settings.smp.semitones + cs.pitchMod,
                              bufL, bufR, n, feed, (int) (&v - voices.data()));
        }

        const float amp = v.velocity * 0.3f * cs.volumeMod;

        if (filterOn)
        {
            const float envOctaves = settings.filterEnv * v.env.getLevel() * 6.0f;
            v.filter.setCutoff (settings.cutoff * std::exp2 (envOctaves + cs.filterMod), filterK, sampleRate);

            for (int i = 0; i < n; ++i)
            {
                const float e = v.env.next (envCoefs) * amp;
                float l = bufL[i], r = bufR[i];
                v.filter.process (l, r);
                outL[i] += l * e;
                outR[i] += r * e;
            }
        }
        else
        {
            for (int i = 0; i < n; ++i)
            {
                const float e = v.env.next (envCoefs) * amp;
                outL[i] += bufL[i] * e;
                outR[i] += bufR[i] * e;
            }
        }

        activityPeak = std::max (activityPeak, v.env.getLevel() * v.velocity);

        if (! v.isActive())
        {
            if (feed != nullptr)
                feed->voicePos[(size_t) (&v - voices.data())].store (-1.0f, std::memory_order_relaxed);
            v.note = v.key = -1;
            v.gate = v.sustained = false;
        }
    }

    //==============================================================================
    std::array<Voice, maxPolyphony> voices {};
    SynthSettings settings;
    EnvCoefs envCoefs;
    const float* sine = sineTable();
    std::unique_ptr<juce::dsp::FFT> fft;
    const SampleData* sample = nullptr;
    SampleVisualFeed* feed = nullptr;

    float sampleRate = 44100.0f, invSampleRate = 1.0f / 44100.0f;
    float filterK = 2.0f;
    bool filterOn = false;
    float lfoPhase = 0.0f, pitchBend = 0.0f;
    float activity = 0.0f, activityPeak = 0.0f;
    bool sustainPedal = false;
    int lastKey = -1;
    uint32_t noteCounter = 0;
};
