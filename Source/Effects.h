#pragma once

#include "DspUtils.h"
#include "SampleData.h"

// Мини-эффекты для звукового дизайна. Каждый — «урезанная» идея известного плагина:
//   Texture  — атмосфера: ветер, винил, дождь, радиоэфир, океан, огонь или свой сэмпл.
//              Звук генерируется на лету, seed и variation делают каждый «дубль» другим.
//   Drive    — сатурация, вейвфолдер, фузз (Thermal / Rift)
//   Formant  — гласные «а-э-и-о-у» поверх звука (VocalSynth / Manipulator)
//   Crusher  — биткрашер: меньше бит и пониженная частота дискретизации
//   Motion   — ритмический гейт под темп проекта (Movement / Effectrix)
//   Crystal  — реверс-гранулярное эхо с питчем (Crystallizer)
//   Space    — шиммер-реверберация с заморозкой хвоста (Valhalla Shimmer / Supermassive)
// Любой эффект с нулевой ручкой не обрабатывается вовсе и не тратит процессор.

struct FxSettings
{
    int textureType = 0;
    float textureAmount = 0.0f, textureTone = 0.6f, textureDensity = 0.5f;
    float textureVariation = 0.5f, textureWidth = 0.8f;
    int textureSeed = 1;
    bool textureAlways = false;
    float drive = 0.0f;
    int driveMode = 0;
    float vowel = 0.0f, vowelMix = 0.0f;
    float crushBits = 8.0f, crushRate = 12000.0f, crushMix = 0.0f;
    float motionDepth = 0.0f;
    int motionRate = 2, motionShape = 1;
    float crystalMix = 0.0f, crystalFeedback = 0.4f;
    int crystalPitch = 0;
    float spaceMix = 0.0f, spaceSize = 0.6f, spacePredelay = 0.02f, shimmer = 0.3f;
    int spaceColor = 0;
    bool freeze = false;
};

struct Transport
{
    double bpm = 120.0;
    double ppq = 0.0;
    bool isPlaying = false;
};

namespace fx
{
    // Сухой/мокрый без провала громкости: до 50% сухой звук остаётся полным,
    // дальше плавно уходит, а эффект к 50% уже звучит в полную силу
    inline float dryGain (float mix) noexcept { return std::min (1.0f, 2.0f - 2.0f * mix); }
    inline float wetGain (float mix) noexcept { return std::min (1.0f, 2.0f * mix); }

    // Мягкий предохранитель для петель обратной связи: почти не трогает обычный уровень
    inline float guard (float x) noexcept { return x / (1.0f + 0.02f * x * x); }

    //==============================================================================
    class Texture
    {
    public:
        void prepare (float newSampleRate)
        {
            sampleRate = newSampleRate;
            followAttack  = 1.0f - std::exp (-1.0f / (0.01f * sampleRate));
            followRelease = 1.0f - std::exp (-1.0f / (0.6f * sampleRate));
            reset();
        }

        void reset()
        {
            currentKey = -1;   // заставит заново «посеять» характер звука
            follower = 0.0f;
        }

        void process (float* left, float* right, int numSamples, const FxSettings& s, float activity,
                      const SampleData* sample)
        {
            if (s.textureAmount <= 0.0f)
            {
                follower = 0.0f;
                return;
            }

            const int key = s.textureSeed * 16 + s.textureType;
            if (key != currentKey)
                reseed (s.textureSeed);
            currentKey = key;

            static constexpr float typeGain[] { 0.9f, 5.0f, 1.45f, 0.8f, 0.9f, 0.9f, 0.9f };
            const float gain = s.textureAmount * typeGain[juce::jlimit (0, 6, s.textureType)];
            const float target = s.textureAlways ? 1.0f : std::min (1.0f, activity * 1.5f);

            for (int start = 0; start < numSamples; start += 32)
            {
                const int n = std::min (32, numSamples - start);
                float buf[2][32];

                for (size_t c = 0; c < 2; ++c)
                {
                    auto& ch = channels[c];
                    control (ch, s, n);
                    for (int i = 0; i < n; ++i)
                        buf[c][i] = ch.tone.lowpass (generate (ch, s, sample));
                }

                for (int i = 0; i < n; ++i)
                {
                    follower += (target - follower) * (target > follower ? followAttack : followRelease);
                    const float g = gain * follower;
                    const float mid  = 0.5f * (buf[0][i] + buf[1][i]);
                    const float side = 0.5f * (buf[0][i] - buf[1][i]) * s.textureWidth;
                    left[start + i]  += (mid + side) * g;
                    right[start + i] += (mid - side) * g;
                }
            }
        }

    private:
        struct Grain
        {
            double pos = 0.0, inc = 1.0;
            int age = 0, length = 1;
            bool active = false;
        };

        struct Channel
        {
            Random rng;
            Svf band, drop, tone;
            OnePole low, hiss;
            float gust = 0.5f, gustTarget = 0.5f, gustCoef = 0.001f;
            int gustTimer = 0;
            float crackle = 0.0f, pop = 0.0f, dropEnv = 0.0f, burst = 0.0f, swell = 0.0f;
            bool gate = false;
            double wavePhase = 0.0, whistlePhase = 0.0;
            std::array<Grain, 2> grains {};
            int grainTimer = 0, nextGrain = 0;
        };

        // «Характер» записи: задаётся seed, одинаковый для обоих каналов
        struct Character
        {
            float baseHz = 200.0f, rangeOct = 3.0f, q = 3.0f, crackleDecay = 0.7f;
            float dropLo = 1500.0f, dropHi = 5000.0f, dropDecay = 0.93f, whistleHz = 1200.0f;
        };

        void reseed (int seed)
        {
            Random r;
            r.seed ((uint32_t) seed * 977u + 13u);
            character.baseHz       = 120.0f + 280.0f * r.unipolar();
            character.rangeOct     = 2.0f + 2.5f * r.unipolar();
            character.q            = 1.5f + 3.5f * r.unipolar();
            character.crackleDecay = 0.55f + 0.3f * r.unipolar();
            character.dropLo       = 900.0f + 2000.0f * r.unipolar();
            character.dropHi       = character.dropLo + 1500.0f + 3500.0f * r.unipolar();
            character.dropDecay    = 0.88f + 0.09f * r.unipolar();
            character.whistleHz    = 500.0f + 2500.0f * r.unipolar();
            const float waveOffset = r.unipolar();

            for (size_t c = 0; c < 2; ++c)
            {
                auto& ch = channels[c];
                ch = Channel {};
                ch.rng.seed ((uint32_t) seed * 31u + (uint32_t) c * 7u + 1u);
                ch.low.setCutoff (150.0f, sampleRate);
                ch.hiss.setCutoff (3000.0f, sampleRate);
                ch.wavePhase = waveOffset + 0.21 * (double) c;
            }
        }

        // Медленные изменения раз в 32 сэмпла: порывы, волны, настройки фильтров
        void control (Channel& ch, const FxSettings& s, int n)
        {
            if ((ch.gustTimer -= n) <= 0)
            {
                const float interval = (3.0f - 2.8f * s.textureDensity) * (0.5f + ch.rng.unipolar());
                ch.gustTimer = (int) (interval * sampleRate);
                ch.gustTarget = 0.5f + 0.5f * ch.rng.bipolar() * (0.2f + 0.8f * s.textureVariation);
                ch.gustCoef = 1.0f - std::exp (-(float) n / (interval * 0.6f * sampleRate));
            }

            ch.gust += (ch.gustTarget - ch.gust) * ch.gustCoef;
            ch.tone.set (250.0f * std::exp2 (s.textureTone * 6.3f), 0.7f, sampleRate);

            switch (s.textureType)
            {
                case 0:   // ветер: полоса шума «гуляет» по частоте
                    ch.band.set (character.baseHz * std::exp2 (ch.gust * character.rangeOct), character.q, sampleRate);
                    break;

                case 3:   // радиоэфир
                    ch.band.set (1800.0f, 0.8f, sampleRate);
                    break;

                case 4:   // океан: медленные волны
                {
                    ch.wavePhase += (double) n * (0.05 + 0.35 * s.textureDensity) / sampleRate;
                    ch.wavePhase -= std::floor (ch.wavePhase);
                    const float wave = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * (float) ch.wavePhase);
                    ch.swell = std::pow (wave, 1.5f) * (1.0f - 0.5f * s.textureVariation) + ch.gust * 0.5f * s.textureVariation;
                    ch.band.set (200.0f + 2200.0f * ch.swell, 0.7f, sampleRate);
                    break;
                }

                default:
                    break;
            }
        }

        float crackleClick (Channel& ch, float ratePerSecond) noexcept
        {
            if (ch.rng.unipolar() < ratePerSecond / sampleRate)
            {
                const float r = ch.rng.unipolar();
                ch.crackle = 0.25f + 0.75f * r * r;
            }
            const float click = ch.crackle * ch.rng.bipolar();
            ch.crackle *= character.crackleDecay;
            return click;
        }

        float generate (Channel& ch, const FxSettings& s, const SampleData* sample) noexcept
        {
            const float noise = ch.rng.bipolar();
            const float density = s.textureDensity, variation = s.textureVariation;

            switch (s.textureType)
            {
                case 0:   // ветер
                {
                    const float rumble = ch.low.process (noise);
                    return ch.band.bandpass (noise) * (0.25f + 0.75f * ch.gust) * 1.6f
                         + rumble * 1.2f * (0.5f + 0.5f * ch.gust);
                }

                case 1:   // винил: шипение, треск (пачками, если variation высокая) и редкие «бумы»
                {
                    const float hiss = (noise - ch.hiss.process (noise)) * 0.08f;
                    const float burstiness = 1.0f - variation + variation * 2.0f * ch.gust;
                    const float click = crackleClick (ch, (3.0f + density * 80.0f) * burstiness);

                    if (ch.rng.unipolar() < (0.2f + density * 2.0f) / sampleRate)
                        ch.pop = 0.6f + 0.4f * ch.rng.unipolar();
                    const float pop = ch.low.process (ch.pop * noise * 6.0f);
                    ch.pop *= 0.97f;
                    return hiss + click * 1.2f + pop;
                }

                case 2:   // дождь: капли разной высоты + шелест; интенсивность «плывёт»
                {
                    if (ch.rng.unipolar() < (30.0f + density * 600.0f) * (0.5f + ch.gust) / sampleRate)
                    {
                        const float r = ch.rng.unipolar();
                        ch.dropEnv = 0.15f + 0.85f * std::pow (r, 1.0f + 2.0f * (1.0f - variation));
                        ch.drop.set (character.dropLo + (character.dropHi - character.dropLo) * ch.rng.unipolar(), 5.0f, sampleRate);
                    }
                    const float drop = ch.drop.bandpass (ch.dropEnv * noise);
                    ch.dropEnv *= character.dropDecay;
                    return drop * 3.0f + ch.hiss.process (noise) * 0.25f * (0.5f + ch.gust);
                }

                case 3:   // радиоэфир: шум включается рывками, поверх свистит гетеродин
                {
                    if (ch.rng.unipolar() < (1.0f + density * 25.0f) / sampleRate)
                        ch.gate = ! ch.gate;
                    ch.burst += ((ch.gate ? 1.0f : 0.15f) - ch.burst) * 0.01f;

                    ch.whistlePhase += character.whistleHz * (1.0f + (ch.gust - 0.5f) * variation * 1.5f) / sampleRate;
                    ch.whistlePhase -= std::floor (ch.whistlePhase);
                    const float whistle = std::sin (juce::MathConstants<float>::twoPi * (float) ch.whistlePhase) * 0.12f;
                    return ch.band.bandpass (noise) * ch.burst * 0.9f + whistle + crackleClick (ch, 4.0f) * 0.6f;
                }

                case 4:   // океан
                    return ch.band.lowpass (noise) * ch.swell * 2.0f + (noise - ch.hiss.process (noise)) * 0.04f * ch.swell;

                case 5:   // огонь: гул пламени + треск поленьев
                {
                    const float roar = ch.low.process (noise) * 3.0f * (0.4f + 0.6f * ch.gust);
                    if (ch.rng.unipolar() < (8.0f + density * 150.0f) * (0.5f + ch.gust) / sampleRate)
                    {
                        ch.crackle = 0.3f + 0.7f * ch.rng.unipolar();
                        ch.drop.set (1500.0f + 5000.0f * ch.rng.unipolar(), 3.0f, sampleRate);
                    }
                    const float crack = ch.drop.bandpass (ch.crackle * noise);
                    ch.crackle *= 0.6f + 0.25f * character.crackleDecay;
                    return roar + crack * 2.5f + (noise - ch.hiss.process (noise)) * 0.03f;
                }

                default:  // свой сэмпл: две длинные гранулы со случайных мест, поэтому петля не повторяется
                    return sample != nullptr ? sampleGrains (ch, s, *sample) : 0.0f;
            }
        }

        float sampleGrains (Channel& ch, const FxSettings& s, const SampleData& sample) noexcept
        {
            const int length = std::max (256, (int) ((4.0f - 3.6f * s.textureDensity) * sampleRate));

            if (--ch.grainTimer <= 0)
            {
                ch.grainTimer = length / 2;
                auto& g = ch.grains[(size_t) ch.nextGrain];
                ch.nextGrain ^= 1;

                const float detune = s.textureVariation * ch.rng.bipolar() * 2.0f;   // до ±2 полутонов
                g.inc = sample.sampleRate / sampleRate * std::exp2 (detune / 12.0f);
                if (ch.rng.unipolar() < s.textureVariation * 0.3f)
                    g.inc = -g.inc;
                g.pos = ch.rng.unipolar() * sample.length();
                g.age = 0;
                g.length = length;
                g.active = true;
            }

            float out = 0.0f;
            for (auto& g : ch.grains)
            {
                if (! g.active)
                    continue;
                out += sample.readWrapped (g.pos) * hann ((float) g.age / (float) g.length);
                g.pos += g.inc;
                if (++g.age >= g.length)
                    g.active = false;
            }
            return out;
        }

        std::array<Channel, 2> channels;
        Character character;
        float sampleRate = 44100.0f;
        float followAttack = 0.01f, followRelease = 0.001f, follower = 0.0f;
        int currentKey = -1;
    };

    //==============================================================================
    class Drive
    {
    public:
        void process (float* left, float* right, int numSamples, float amount, int mode)
        {
            if (amount <= 0.0f)
                return;

            const float wet = std::min (1.0f, amount * 4.0f);   // плавное включение от нуля

            for (int i = 0; i < numSamples; ++i)
            {
                float l = left[i], r = right[i];

                switch (mode)
                {
                    case 0:   // тёплая сатурация
                    {
                        const float g = 1.0f + amount * 12.0f, comp = 1.0f / (1.0f + amount * 1.2f);
                        l = softClip (l * g) * comp;
                        r = softClip (r * g) * comp;
                        break;
                    }

                    case 1:   // вейвфолдер: громкие места «заворачиваются» и звенят
                    {
                        const float g = 1.0f + amount * 6.0f;
                        l = std::sin (l * g) * 0.8f;
                        r = std::sin (r * g) * 0.8f;
                        break;
                    }

                    default:  // фузз: жёсткий несимметричный клип, как перегруженная педаль
                    {
                        const float g = 1.0f + amount * 30.0f;
                        l = juce::jlimit (-0.55f, 0.8f, l * g);
                        r = juce::jlimit (-0.55f, 0.8f, r * g);
                        // убираем постоянную составляющую от несимметрии
                        const float outL = l - dcInL + 0.995f * dcOutL;
                        const float outR = r - dcInR + 0.995f * dcOutR;
                        dcInL = l; dcOutL = outL;
                        dcInR = r; dcOutR = outR;
                        l = outL;
                        r = outR;
                        break;
                    }
                }

                left[i]  += (l - left[i])  * wet;
                right[i] += (r - right[i]) * wet;
            }
        }

    private:
        float dcInL = 0.0f, dcOutL = 0.0f, dcInR = 0.0f, dcOutR = 0.0f;
    };

    class Formant
    {
    public:
        void prepare (float newSampleRate) { sampleRate = newSampleRate; reset(); }

        void reset()
        {
            for (auto& f : filters)
                f.reset();
        }

        void process (float* left, float* right, int numSamples, float vowel, float mix)
        {
            if (mix <= 0.0f)
                return;

            // Форманты (Гц) для гласных a, e, i, o, u
            static constexpr float table[5][3] { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                                 { 570, 840, 2410 },  { 300, 870, 2240 } };
            static constexpr float q[3] { 7.0f, 10.0f, 12.0f };
            static constexpr float gain[3] { 1.0f, 0.6f, 0.3f };

            for (int start = 0; start < numSamples; start += 32)
            {
                const int n = std::min (32, numSamples - start);
                smoothed += (vowel - smoothed) * 0.15f;

                const float pos = smoothed * 4.0f;
                const int a = std::min (3, (int) pos);
                const float t = pos - (float) a;

                for (size_t k = 0; k < 3; ++k)
                {
                    const float hz = table[a][k] * std::pow (table[a + 1][k] / table[a][k], t);
                    filters[k].set (hz, q[k], sampleRate);
                    filters[k + 3].set (hz, q[k], sampleRate);
                }

                for (int i = start; i < start + n; ++i)
                {
                    float wl = 0.0f, wr = 0.0f;
                    for (size_t k = 0; k < 3; ++k)
                    {
                        wl += filters[k].bandpass (left[i]) * gain[k];
                        wr += filters[k + 3].bandpass (right[i]) * gain[k];
                    }
                    left[i]  += (wl * 1.8f - left[i])  * mix;
                    right[i] += (wr * 1.8f - right[i]) * mix;
                }
            }
        }

    private:
        std::array<Svf, 6> filters;   // 3 форманты × 2 канала
        float sampleRate = 44100.0f, smoothed = 0.0f;
    };

    //==============================================================================
    class Motion
    {
    public:
        void prepare (float newSampleRate)
        {
            sampleRate = newSampleRate;
            smoothing = 1.0f - std::exp (-1.0f / (0.003f * sampleRate));   // 3 мс — без щелчков
        }

        void process (float* left, float* right, int numSamples, float depth, int rateIndex, int shape,
                      const Transport& transport)
        {
            static constexpr double stepsPerBeat[] { 1.0, 2.0, 4.0, 8.0 };
            const double steps = stepsPerBeat[juce::jlimit (0, 3, rateIndex)];

            // Когда проект играет, привязываемся к сетке FL Studio
            if (transport.isPlaying)
                phase = (float) (transport.ppq * steps - std::floor (transport.ppq * steps));

            const float inc = (float) (std::max (20.0, transport.bpm) / 60.0 * steps) / sampleRate;

            if (depth <= 0.0f)
            {
                phase = std::fmod (phase + inc * (float) numSamples, 1.0f);
                gain = 1.0f;
                return;
            }

            for (int i = 0; i < numSamples; ++i)
            {
                float target;
                switch (shape)
                {
                    case 0:  target = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase); break;
                    case 1:  target = phase < 0.5f ? 1.0f : 0.0f; break;
                    default: { const float d = 1.0f - phase; target = 1.0f - d * d * d; break; }   // «насос»
                }

                gain += (target - gain) * smoothing;
                const float g = 1.0f - depth * (1.0f - gain);
                left[i] *= g;
                right[i] *= g;

                phase += inc;
                if (phase >= 1.0f)
                    phase -= 1.0f;
            }
        }

    private:
        float sampleRate = 44100.0f, smoothing = 0.01f;
        float phase = 0.0f, gain = 1.0f;
    };


    //==============================================================================
    // Биткрашер: меньше бит = «ступеньки» громкости, ниже частота = металлический алиасинг
    class Crusher
    {
    public:
        void prepare (float newSampleRate) { sampleRate = newSampleRate; }

        void process (float* left, float* right, int numSamples, float bits, float rateHz, float mix)
        {
            if (mix <= 0.0f)
                return;

            const float levels = std::exp2 (bits - 1.0f);
            const float step = std::min (1.0f, rateHz / sampleRate);

            for (int i = 0; i < numSamples; ++i)
            {
                phase += step;
                if (phase >= 1.0f)
                {
                    phase -= 1.0f;
                    heldL = std::round (left[i] * levels) / levels;
                    heldR = std::round (right[i] * levels) / levels;
                }

                left[i]  += (heldL - left[i])  * mix;
                right[i] += (heldR - right[i]) * mix;
            }
        }

    private:
        float sampleRate = 44100.0f, phase = 1.0f, heldL = 0.0f, heldR = 0.0f;
    };

    //==============================================================================
    // Реверс-гранулярное эхо: звук режется на кусочки, которые играются задом наперёд
    // и с другим питчем. С обратной связью даёт «кристальные» поднимающиеся хвосты.
    class Crystal
    {
    public:
        void prepare (float newSampleRate)
        {
            sampleRate = newSampleRate;
            grainLength = { 0.22f * sampleRate, 0.29f * sampleRate };   // разные длины = широкое стерео
            for (auto& b : buffers)
                b.setMinimumSize ((int) (sampleRate * 1.6f));
            reset();
        }

        void reset()
        {
            for (auto& b : buffers)
                b.clear();
            phase = { 0.0f, 0.0f };
        }

        void process (float* left, float* right, int numSamples, float mix, float feedback, int pitchIndex)
        {
            if (mix <= 0.0f)
            {
                needsClear = true;
                return;
            }

            if (needsClear)
            {
                reset();
                needsClear = false;
            }

            static constexpr float ratios[] { 2.0f, 1.5f, 0.5f, 1.0f };
            const float ratio = ratios[juce::jlimit (0, 3, pitchIndex)];
            const float dry = dryGain (mix), wet = wetGain (mix) * 1.3f;
            float* io[] { left, right };

            for (size_t c = 0; c < 2; ++c)
            {
                auto& buf = buffers[c];
                const float len = grainLength[c];
                const float inc = 1.0f / len;
                float p = phase[c];
                float* x = io[c];

                for (int i = 0; i < numSamples; ++i)
                {
                    float grains = 0.0f;
                    for (float offset : { 0.0f, 0.5f })
                    {
                        float pk = p + offset;
                        if (pk >= 1.0f) pk -= 1.0f;
                        // гранула стартует с самого свежего звука и бежит назад со скоростью ratio
                        grains += hann (pk) * buf.atFractional (1.0f + (1.0f + ratio) * pk * len);
                    }

                    buf.push (x[i] + softClip (grains * feedback));
                    x[i] = x[i] * dry + grains * wet;

                    p += inc;
                    if (p >= 1.0f) p -= 1.0f;
                }

                phase[c] = p;
            }
        }

    private:
        std::array<DelayBuffer, 2> buffers;
        std::array<float, 2> grainLength {}, phase {};
        float sampleRate = 44100.0f;
        bool needsClear = false;
    };

    //==============================================================================
    // Реверберация на 4 линиях задержки (FDN) с предзадержкой и медленной модуляцией
    // (убирает «металл»). Shimmer — питч +12 полутонов внутри обратной связи,
    // поэтому хвост постоянно «поднимается» вверх. Freeze держит хвост бесконечно.
    class Space
    {
    public:
        void prepare (float newSampleRate)
        {
            sampleRate = newSampleRate;
            const float scale = sampleRate / 44100.0f;

            static constexpr int lineLengths[] { 3089, 3457, 3851, 4219 };
            static constexpr int diffuserLengths[] { 142, 379, 107, 277 };

            for (size_t i = 0; i < 4; ++i)
            {
                lengths[i] = (int) ((float) lineLengths[i] * scale);
                lines[i].setMinimumSize (lengths[i] + 64);
                diffLengths[i] = (int) ((float) diffuserLengths[i] * scale);
                diffusers[i].setMinimumSize (diffLengths[i] + 1);
            }

            for (auto& p : predelay)
                p.setMinimumSize ((int) (0.31f * sampleRate));

            shiftWindow = 0.05f * sampleRate;
            shifter.setMinimumSize ((int) shiftWindow + 4);

            const float w = juce::MathConstants<float>::twoPi * 0.5f / sampleRate;   // модуляция 0.5 Гц
            rotCos = std::cos (w);
            rotSin = std::sin (w);
            reset();
        }

        void reset()
        {
            for (auto& l : lines) l.clear();
            for (auto& d : diffusers) d.clear();
            for (auto& p : predelay) p.clear();
            shifter.clear();
            damp = {};
            shiftPhase = 0.0f;
            modC = 1.0f;
            modS = 0.0f;
        }

        void process (float* left, float* right, int numSamples, const FxSettings& s)
        {
            if (s.spaceMix <= 0.0f)
            {
                needsClear = true;
                return;
            }

            if (needsClear)
            {
                reset();
                needsClear = false;
            }

            // Размер задаёт время затухания: от 0.5 до 15 секунд
            const float decaySeconds = 0.5f + s.spaceSize * s.spaceSize * 14.5f;
            std::array<float, 4> gains {};
            for (size_t i = 0; i < 4; ++i)
                gains[i] = s.freeze ? 1.0f : std::pow (10.0f, -3.0f * (float) lengths[i] / (decaySeconds * sampleRate));

            const float dampCoef    = s.freeze ? 1.0f : (s.spaceColor == 0 ? 0.8f : 0.3f);   // bright / dark
            const float shimmerGain = s.freeze ? 0.0f : s.shimmer * 0.55f;
            const float inputGain   = s.freeze ? 0.0f : 0.5f;
            const float dry = dryGain (s.spaceMix);
            const float wet = wetGain (s.spaceMix) * wetCalibration;
            const int pre = std::max (1, (int) (s.spacePredelay * sampleRate));

            for (int i = 0; i < numSamples; ++i)
            {
                predelay[0].push (left[i]);
                predelay[1].push (right[i]);
                const float inL = diffuse (predelay[0].at (pre) * inputGain, 0);
                const float inR = diffuse (predelay[1].at (pre) * inputGain, 2);

                // Медленный «хорус» в двух линиях: хвост живой, без звона
                const float c = modC * rotCos - modS * rotSin;
                modS = modS * rotCos + modC * rotSin;
                modC = c;

                std::array<float, 4> o {};
                o[0] = lines[0].atFractional ((float) lengths[0] - 12.0f - 10.0f * modS);
                o[1] = lines[1].atFractional ((float) lengths[1] - 12.0f - 10.0f * modC);
                o[2] = lines[2].at (lengths[2]);
                o[3] = lines[3].at (lengths[3]);

                for (size_t k = 0; k < 4; ++k)
                {
                    damp[k] += (o[k] - damp[k]) * dampCoef;
                    o[k] = damp[k];
                }

                // Матрица Адамара перемешивает линии без потери энергии
                const float h0 = 0.5f * (o[0] + o[1] + o[2] + o[3]);
                const float h1 = 0.5f * (o[0] - o[1] + o[2] - o[3]);
                const float h2 = 0.5f * (o[0] + o[1] - o[2] - o[3]);
                const float h3 = 0.5f * (o[0] - o[1] - o[2] + o[3]);

                const float sh = shimmerGain > 0.0f ? pitchUp ((o[0] + o[1] + o[2] + o[3]) * 0.25f) * shimmerGain : 0.0f;

                lines[0].push (guard (h0 * gains[0] + inL + sh));
                lines[1].push (guard (h1 * gains[1] + inR + sh));
                lines[2].push (guard (h2 * gains[2] + inL - sh));
                lines[3].push (guard (h3 * gains[3] + inR - sh));

                left[i]  = left[i]  * dry + (o[0] + o[2]) * wet;
                right[i] = right[i] * dry + (o[1] + o[3]) * wet;
            }
        }

    private:
        static constexpr float wetCalibration = 1.0f;

        // Два последовательных allpass-фильтра «размывают» атаку перед ревербом
        float diffuse (float x, size_t first) noexcept
        {
            for (size_t k = first; k < first + 2; ++k)
            {
                const float d = diffusers[k].at (diffLengths[k]);
                const float w = x - 0.6f * d;
                diffusers[k].push (w);
                x = d + 0.6f * w;
            }
            return x;
        }

        // Питч-шифтер на октаву вверх: две «читающие головки» с плавной перекрёсткой
        float pitchUp (float x) noexcept
        {
            shifter.push (x);
            shiftPhase += 1.0f / shiftWindow;   // (ratio - 1) / window, ratio = 2
            if (shiftPhase >= 1.0f) shiftPhase -= 1.0f;

            float out = 0.0f;
            for (float offset : { 0.0f, 0.5f })
            {
                float p = shiftPhase + offset;
                if (p >= 1.0f) p -= 1.0f;
                out += hann (p) * shifter.atFractional (1.0f + (1.0f - p) * shiftWindow);
            }
            return out;
        }

        std::array<DelayBuffer, 4> lines, diffusers;
        std::array<DelayBuffer, 2> predelay;
        std::array<int, 4> lengths {}, diffLengths {};
        std::array<float, 4> damp {};
        DelayBuffer shifter;
        float shiftWindow = 2205.0f, shiftPhase = 0.0f;
        float rotCos = 1.0f, rotSin = 0.0f, modC = 1.0f, modS = 0.0f;
        float sampleRate = 44100.0f;
        bool needsClear = false;
    };
}

//==============================================================================
class FxChain
{
public:
    void prepare (double sampleRate)
    {
        const auto sr = (float) sampleRate;
        texture.prepare (sr);
        formant.prepare (sr);
        crusher.prepare (sr);
        motion.prepare (sr);
        crystal.prepare (sr);
        space.prepare (sr);
    }

    void process (juce::AudioBuffer<float>& buffer, const FxSettings& s, float activity, const Transport& transport,
                  const SampleData* textureSample)
    {
        float* l = buffer.getWritePointer (0);
        float* r = buffer.getWritePointer (1);
        const int n = buffer.getNumSamples();

        drive.process (l, r, n, s.drive, s.driveMode);
        formant.process (l, r, n, s.vowel, s.vowelMix);
        crusher.process (l, r, n, s.crushBits, s.crushRate, s.crushMix);
        texture.process (l, r, n, s, activity, textureSample);
        motion.process (l, r, n, s.motionDepth, s.motionRate, s.motionShape, transport);
        crystal.process (l, r, n, s.crystalMix, s.crystalFeedback, s.crystalPitch);
        space.process (l, r, n, s);
    }

private:
    fx::Drive drive;
    fx::Formant formant;
    fx::Crusher crusher;
    fx::Texture texture;
    fx::Motion motion;
    fx::Crystal crystal;
    fx::Space space;
};
