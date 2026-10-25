#include "SampleData.h"
#include "DspUtils.h"
#include <juce_dsp/juce_dsp.h>

void SampleData::analyse()
{
    juce::dsp::FFT fft (fftOrder);
    std::vector<float> window ((size_t) fftSize), buffer ((size_t) fftSize * 2), prevPhase ((size_t) numBins, 0.0f);

    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize);

    const int len = length();
    numFrames = std::max (1, (len + hop - 1) / hop);
    mags.assign ((size_t) numFrames * numBins, 0.0f);
    freqs.assign ((size_t) numFrames * numBins, 0.0f);

    const double binHz = sampleRate / fftSize;
    const double expected = juce::MathConstants<double>::twoPi * hop / fftSize;   // набег фазы на бин за hop

    for (int f = 0; f < numFrames; ++f)
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        const int start = f * hop - fftSize / 2;   // кадр центрирован на f * hop

        for (int i = 0; i < fftSize; ++i)
        {
            const int idx = start + i;
            if (idx >= 0 && idx < len)
                buffer[(size_t) i] = audio[(size_t) idx] * window[(size_t) i];
        }

        fft.performRealOnlyForwardTransform (buffer.data(), true);

        float* m  = mags.data()  + (size_t) f * numBins;
        float* fr = freqs.data() + (size_t) f * numBins;

        for (int k = 0; k < numBins; ++k)
        {
            const float re = buffer[(size_t) k * 2], im = buffer[(size_t) k * 2 + 1];
            const float phase = std::atan2 (im, re);
            m[k] = std::sqrt (re * re + im * im);

            // Отклонение фазы от ожидаемого даёт точную частоту внутри бина
            double dp = phase - prevPhase[(size_t) k] - k * expected;
            dp -= juce::MathConstants<double>::twoPi * std::round (dp / juce::MathConstants<double>::twoPi);
            prevPhase[(size_t) k] = phase;

            fr[k] = f == 0 ? (float) (k * binHz)
                           : (float) ((k + dp / expected) * binHz);
        }
    }
}

void SampleData::finalise()
{
    float peak = 0.0f;
    for (auto s : audio)
        peak = std::max (peak, std::abs (s));

    if (peak > 0.0f)
        for (auto& s : audio)
            s *= 0.9f / peak;

    // Короткие фейды убирают щелчки на краях
    const int fade = std::min (64, length() / 2);
    for (int i = 0; i < fade; ++i)
    {
        const float g = (float) i / (float) fade;
        audio[(size_t) i] *= g;
        audio[audio.size() - 1 - (size_t) i] *= g;
    }

    if (audio.size() < 2)
        audio.resize (2, 0.0f);

    analyse();
}

std::unique_ptr<SampleData> SampleData::fromFile (const juce::File& file, juce::String& error)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "unsupported file";
        return nullptr;
    }

    // Берём не больше 30 секунд: этого хватает для текстур и гранулярки
    const auto maxLength = (juce::int64) (reader->sampleRate * 30.0);
    const int length = (int) std::min (reader->lengthInSamples, maxLength);
    if (length < 64)
    {
        error = "file is too short";
        return nullptr;
    }

    juce::AudioBuffer<float> buffer ((int) reader->numChannels, length);
    reader->read (&buffer, 0, length, 0, true, true);

    auto data = std::make_unique<SampleData>();
    data->name = file.getFileNameWithoutExtension();
    data->sampleRate = reader->sampleRate;
    data->audio.assign ((size_t) length, 0.0f);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* src = buffer.getReadPointer (ch);
        for (int i = 0; i < length; ++i)
            data->audio[(size_t) i] += src[i] / (float) buffer.getNumChannels();
    }

    data->finalise();
    return data;
}

//==============================================================================
namespace
{
    constexpr double genRate = 44100.0;
    constexpr double c4 = 261.6256;
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    std::unique_ptr<SampleData> makeSample (const char* name, double seconds)
    {
        auto d = std::make_unique<SampleData>();
        d->name = name;
        d->sampleRate = genRate;
        d->audio.assign ((size_t) (seconds * genRate), 0.0f);
        return d;
    }

    float polySaw (double& phase, double inc)
    {
        phase += inc;
        if (phase >= 1.0) phase -= 1.0;
        const float t = (float) phase, dt = (float) inc;
        float s = 2.0f * t - 1.0f;
        if (t < dt)             { const float x = t / dt;          s -= x + x - x * x - 1.0f; }
        else if (t > 1.0f - dt) { const float x = (t - 1.0f) / dt; s -= x * x + x + x + 1.0f; }
        return s;
    }

    // Три форманты гласной: a, o, i, e, u
    struct Vowels
    {
        static constexpr float table[5][3] { { 730, 1090, 2440 }, { 570, 840, 2410 }, { 270, 2290, 3010 },
                                             { 530, 1840, 2480 }, { 300, 870, 2240 } };
        std::array<fx::Svf, 3> f;

        float process (float x, float vowelPos)
        {
            const int a = juce::jlimit (0, 3, (int) vowelPos);
            const float t = juce::jlimit (0.0f, 1.0f, vowelPos - (float) a);
            static constexpr float gains[3] { 1.0f, 0.55f, 0.3f };
            float out = 0.0f;
            for (size_t k = 0; k < 3; ++k)
            {
                f[k].set (table[a][k] + t * (table[a + 1][k] - table[a][k]), 8.0f + 3.0f * (float) k, (float) genRate);
                out += f[k].bandpass (x) * gains[k];
            }
            return out;
        }
    };

    // FM-колокольчик со стеклянным звоном
    std::unique_ptr<SampleData> makeGlass()
    {
        auto d = makeSample ("glass", 3.5);
        double pc = 0, pm = 0, p2 = 0, p3 = 0;
        for (size_t i = 0; i < d->audio.size(); ++i)
        {
            const double t = (double) i / genRate;
            pm += c4 * 3.5 / genRate;
            pc += c4 / genRate;
            p2 += c4 * 2.0 / genRate;
            p3 += c4 * 4.21 / genRate;
            const double mod = 2.4 * std::exp (-t * 3.0) * std::sin (twoPi * pm);
            d->audio[i] = (float) (0.6 * std::sin (twoPi * pc + mod) * std::exp (-t * 1.1)
                                 + 0.25 * std::sin (twoPi * p2) * std::exp (-t * 2.0)
                                 + 0.15 * std::sin (twoPi * p3) * std::exp (-t * 4.0));
        }
        d->finalise();
        return d;
    }

    // Хор: расстроенные пилы через форманту «а» с вибрато
    std::unique_ptr<SampleData> makeChoir()
    {
        auto d = makeSample ("choir", 4.0);
        const double detune[] { -0.011, -0.004, 0.0, 0.005, 0.012 };
        double phases[5] {}, vib = 0;
        Vowels vowels;
        fx::Random rng;
        const auto n = d->audio.size();

        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / genRate;
            vib += 5.2 / genRate;
            const double f = c4 * (1.0 + 0.004 * std::sin (twoPi * vib));
            float src = 0.0f;
            for (int v = 0; v < 5; ++v)
                src += polySaw (phases[v], f * (1.0 + detune[v]) / genRate) * 0.2f;
            src += rng.bipolar() * 0.03f;

            const float env = (float) std::min ({ 1.0, t / 0.35, (4.0 - t) / 0.6 });
            d->audio[i] = vowels.process (src, 0.3f + 0.25f * (float) std::sin (twoPi * 0.15 * t)) * env;
        }
        d->finalise();
        return d;
    }

    // Металл: негармонические обертоны, как у гонга или гамелана
    std::unique_ptr<SampleData> makeMetal()
    {
        auto d = makeSample ("metal", 3.5);
        const double ratios[] { 1.0, 2.76, 5.40, 8.93, 13.34, 18.64 };
        const double amps[]   { 1.0, 0.6, 0.45, 0.3, 0.2, 0.12 };
        const double decays[] { 0.7, 1.1, 1.7, 2.3, 3.2, 4.2 };
        double phases[6] {};
        fx::Random rng;
        fx::OnePole strike;
        strike.setCutoff (2500.0f, (float) genRate);

        for (size_t i = 0; i < d->audio.size(); ++i)
        {
            const double t = (double) i / genRate;
            double s = 0.0;
            for (int k = 0; k < 6; ++k)
            {
                phases[k] += c4 * ratios[k] / genRate;
                s += amps[k] * std::sin (twoPi * phases[k]) * std::exp (-t * decays[k]);
            }
            s += strike.process (rng.bipolar()) * std::exp (-t * 40.0) * 2.0;
            d->audio[i] = (float) (s * 0.3);
        }
        d->finalise();
        return d;
    }

    // Дыхание: шум с плавающим полосовым фильтром и едва слышным тоном
    std::unique_ptr<SampleData> makeBreath()
    {
        auto d = makeSample ("breath", 4.0);
        fx::Random rng;
        fx::Svf band;
        double p1 = 0, p2 = 0;
        const auto n = d->audio.size();

        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / genRate;
            if ((i & 31) == 0)
                band.set ((float) (500.0 + 1800.0 * (0.5 + 0.5 * std::sin (twoPi * 0.4 * t - 1.2))), 2.0f, (float) genRate);
            p1 += c4 / genRate;
            p2 += c4 * 2.0 / genRate;
            const double swell = std::min ({ 1.0, t / 0.6, (4.0 - t) / 0.8 }) * (0.8 + 0.2 * std::sin (twoPi * 0.9 * t));
            const double s = band.bandpass (rng.bipolar()) * 0.9 + 0.06 * std::sin (twoPi * p1) + 0.03 * std::sin (twoPi * p2);
            d->audio[i] = (float) (s * swell);
        }
        d->finalise();
        return d;
    }

    // Вокальная фраза: гласные меняются, высота ходит по минорной мелодии (вайб вокальных чопов)
    std::unique_ptr<SampleData> makeVox()
    {
        auto d = makeSample ("vox", 3.6);
        const int melody[] { 0, 3, 7, 5, 10, 7 };
        double phase = 0, vib = 0, pitch = 0;
        Vowels vowels;
        fx::Random rng;
        fx::OnePole glottal;
        glottal.setCutoff (1800.0f, (float) genRate);
        const auto n = d->audio.size();

        for (size_t i = 0; i < n; ++i)
        {
            const double t = (double) i / genRate;
            const int note = (int) (t / 0.6);
            const double target = melody[std::min (note, 5)];
            pitch += (target - pitch) * 0.0015;   // портаменто между нотами
            vib += 5.5 / genRate;

            const double f = c4 * std::pow (2.0, (pitch + 0.15 * std::sin (twoPi * vib)) / 12.0);
            const float src = glottal.process (polySaw (phase, f / genRate)) + rng.bipolar() * 0.06f;

            const float notePhase = (float) std::fmod (t, 0.6) / 0.6f;
            const float env = std::min (1.0f, notePhase * 12.0f) * (0.75f + 0.25f * (1.0f - notePhase))
                            * (float) std::min (1.0, (3.6 - t) / 0.3);
            d->audio[i] = vowels.process (src, (float) std::fmod (t * 1.3, 4.0)) * env;
        }
        d->finalise();
        return d;
    }
}

const BuiltinSamples& BuiltinSamples::get()
{
    static const BuiltinSamples instance = []
    {
        BuiltinSamples b;
        b.items = { makeGlass(), makeChoir(), makeMetal(), makeBreath(), makeVox() };
        return b;
    }();
    return instance;
}
