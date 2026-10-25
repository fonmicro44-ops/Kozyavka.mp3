#pragma once

#include "DspUtils.h"
#include "SampleData.h"
#include <juce_dsp/juce_dsp.h>
#include <atomic>

struct SampleSettings
{
    float level = 0.0f, semitones = 0.0f, pan = 0.0f;
    int mode = 1;                    // 0 Classic, 1 Granular, 2 Spectral
    float position = 0.0f, scan = 1.0f;
    bool loop = true, keytrack = true;
    float grainSize = 0.12f, density = 20.0f, spray = 0.1f, jitter = 0.0f, reverse = 0.0f;
    float blur = 0.0f, shiftHz = 0.0f, sparse = 0.0f, whisper = 0.0f;
};

// Данные для анимации в интерфейсе: где играют голоса и где родились гранулы
struct SampleVisualFeed
{
    static constexpr int maxVoices = 16, grainSlots = 64;
    std::array<std::atomic<float>, maxVoices> voicePos {};
    std::array<std::atomic<float>, grainSlots> grainPos {};
    std::atomic<uint32_t> grainWrite { 0 };

    SampleVisualFeed()
    {
        for (auto& p : voicePos) p = -1.0f;
        for (auto& p : grainPos) p = -1.0f;
    }

    void addGrain (float normalisedPos) noexcept
    {
        const auto i = grainWrite.fetch_add (1, std::memory_order_relaxed);
        grainPos[i % grainSlots].store (normalisedPos, std::memory_order_relaxed);
    }
};

//==============================================================================
class SampleVoice
{
public:
    static constexpr int N = SampleData::fftSize, H = SampleData::hop, bins = SampleData::numBins;

    void prepare (double newHostRate, const juce::dsp::FFT* sharedFft, uint32_t seed)
    {
        hostRate = newHostRate;
        fft = sharedFft;
        rng.seed (seed);
        phase.assign (bins, 0.0f);
        blurMag.assign (bins, 0.0f);
        targetMag.assign (bins, 0.0f);
        targetFreq.assign (bins, 0.0f);
        ola.assign (N, 0.0f);
        fftData.assign (N * 2, 0.0f);
        outBuf.assign (H, 0.0f);
        window.assign (N, 0.0f);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) N);
    }

    void start (const SampleData* data, const SampleSettings& s)
    {
        finished = false;
        for (auto& g : grains) g.active = false;
        spawnTimer = 0.0f;

        if (data == nullptr)
            return;

        playPos = scanPos = s.position * (data->length() - 1);
        specPos = s.position * (double) data->numFrames;
        std::fill (ola.begin(), ola.end(), 0.0f);
        std::fill (blurMag.begin(), blurMag.end(), 0.0f);
        for (auto& p : phase) p = rng.unipolar() * juce::MathConstants<float>::twoPi;
        outRead = H;   // первый же сэмпл запустит синтез кадра
    }

    // Добавляет звук сэмпла в bufL/bufR. pitch — высота в полутонах (MIDI-нота с модуляцией)
    void render (const SampleData& data, const SampleSettings& s, float pitch, float* bufL, float* bufR, int n,
                 SampleVisualFeed* feed, int voiceIndex)
    {
        const float ratio = std::exp2 ((pitch - (float) data.rootNote) / 12.0f);
        const float gainL = s.level * std::min (1.0f, 1.0f - s.pan);
        const float gainR = s.level * std::min (1.0f, 1.0f + s.pan);

        switch (s.mode)
        {
            case 0:  renderClassic (data, s, ratio, gainL, gainR, bufL, bufR, n); break;
            case 1:  renderGranular (data, s, ratio, gainL, gainR, bufL, bufR, n, feed); break;
            default: renderSpectral (data, s, ratio, gainL, gainR, bufL, bufR, n); break;
        }

        if (feed != nullptr && voiceIndex < SampleVisualFeed::maxVoices)
        {
            const double pos = s.mode == 0 ? playPos / data.length()
                             : s.mode == 1 ? scanPos / data.length()
                                           : specPos / std::max (1, data.numFrames);
            feed->voicePos[(size_t) voiceIndex].store (finished ? -1.0f : (float) pos, std::memory_order_relaxed);
        }
    }

private:
    //==============================================================================
    // Classic: обычный сэмплер — высота меняется вместе со скоростью
    void renderClassic (const SampleData& data, const SampleSettings& s, float ratio,
                        float gainL, float gainR, float* bufL, float* bufR, int n)
    {
        const double inc = ratio * data.sampleRate / hostRate;
        const double end = data.length() - 1;
        const double loopStart = s.position * end;

        for (int i = 0; i < n && ! finished; ++i)
        {
            if (playPos >= end)
            {
                if (s.loop && end - loopStart > 64.0) playPos = loopStart;
                else { finished = true; break; }
            }

            const float v = data.readWrapped (playPos);
            bufL[i] += v * gainL;
            bufR[i] += v * gainR;
            playPos += inc;
        }
    }

    //==============================================================================
    // Granular: облако коротких кусочков звука вокруг движущейся «головки» scan
    struct Grain
    {
        double pos = 0.0, inc = 1.0;
        int age = 0, length = 1, delay = 0;
        float gainL = 1.0f, gainR = 1.0f;
        bool active = false;
    };

    void renderGranular (const SampleData& data, const SampleSettings& s, float ratio,
                         float gainL, float gainR, float* bufL, float* bufR, int n, SampleVisualFeed* feed)
    {
        const double len = data.length();
        const double srcPerHost = data.sampleRate / hostRate;
        const int grainLength = std::max (32, (int) (s.grainSize * hostRate));
        const float interval = (float) hostRate / std::max (0.5f, s.density);
        // Громкость не зависит от плотности: гранулы складываются по мощности, окно Ханна «съедает» ~4 дБ
        const float norm = std::min (1.3f, 1.6f / std::sqrt (std::max (0.01f, s.density * s.grainSize)));

        // Рождаем новые гранулы внутри этого кусочка
        for (int i = 0; i < n; ++i)
        {
            spawnTimer -= 1.0f;
            if (spawnTimer <= 0.0f)
            {
                spawnTimer += interval * (0.7f + 0.6f * rng.unipolar());
                spawnGrain (data, s, ratio, srcPerHost, grainLength, i, feed);
            }
        }

        scanPos += s.scan * srcPerHost * n;
        scanPos -= std::floor (scanPos / len) * len;

        for (auto& g : grains)
        {
            if (! g.active)
                continue;

            const float gl = g.gainL * gainL * norm, gr = g.gainR * gainR * norm;
            const float invLength = 1.0f / (float) g.length;

            for (int i = g.delay; i < n; ++i)
            {
                const float v = data.readWrapped (g.pos) * fx::hann ((float) g.age * invLength);
                bufL[i] += v * gl;
                bufR[i] += v * gr;
                g.pos += g.inc;

                if (++g.age >= g.length)
                {
                    g.active = false;
                    break;
                }
            }
            g.delay = 0;
        }
    }

    void spawnGrain (const SampleData& data, const SampleSettings& s, float ratio, double srcPerHost,
                     int grainLength, int delay, SampleVisualFeed* feed)
    {
        Grain* g = nullptr;
        for (auto& candidate : grains)
            if (! candidate.active) { g = &candidate; break; }
        if (g == nullptr)
            return;

        // Джиттер прыгает по «музыкальным» интервалам, а не в случайную расстройку
        static constexpr float intervals[] { 12.0f, -12.0f, 7.0f, 19.0f, 24.0f, 5.0f };
        float semis = 0.0f;
        if (rng.unipolar() < s.jitter)
            semis = intervals[(int) (rng.unipolar() * 5.99f)];

        const double len = data.length();
        const double centre = scanPos + s.spray * rng.bipolar() * len * 0.5;
        double inc = ratio * std::exp2 (semis / 12.0f) * srcPerHost;
        const double span = grainLength * inc;

        double startPos = centre - span * 0.5;
        if (rng.unipolar() < s.reverse)
        {
            inc = -inc;
            startPos = centre + span * 0.5;
        }

        const float pan = juce::jlimit (-1.0f, 1.0f, s.spray * rng.bipolar() * 0.8f);
        const float amp = 0.8f + 0.2f * rng.unipolar();

        *g = { startPos, inc, 0, grainLength, delay,
               amp * std::min (1.0f, 1.0f - pan), amp * std::min (1.0f, 1.0f + pan), true };

        if (feed != nullptr)
        {
            double p = centre / len;
            feed->addGrain ((float) (p - std::floor (p)));
        }
    }

    //==============================================================================
    // Spectral: звук собирается из спектра заново (фазовый вокодер). Высота и время
    // независимы: можно заморозить кадр, растянуть, сдвинуть частоты или проредить спектр.
    void renderSpectral (const SampleData& data, const SampleSettings& s, float ratio,
                         float gainL, float gainR, float* bufL, float* bufR, int n)
    {
        if (fft == nullptr || data.numFrames < 1)
            return;

        for (int i = 0; i < n; ++i)
        {
            if (outRead >= H)
            {
                synthesiseFrame (data, s, ratio);
                outRead = 0;
            }

            const float v = outBuf[(size_t) outRead++];
            bufL[i] += v * gainL;
            bufR[i] += v * gainR;
        }
    }

    void synthesiseFrame (const SampleData& data, const SampleSettings& s, float ratio)
    {
        // Сдвигаем позицию по сэмплу: scan = 1 — в реальном времени, 0 — заморозка
        const double framesPerHop = (H / hostRate) * (data.sampleRate / SampleData::hop) * s.scan;
        specPos += framesPerHop;
        specPos -= std::floor (specPos / data.numFrames) * data.numFrames;

        const int f0 = (int) specPos;
        const int f1 = (f0 + 1) % data.numFrames;
        const float frac = (float) (specPos - f0);
        const float* m0 = data.frameMags (f0);
        const float* m1 = data.frameMags (f1);
        const float* q0 = data.frameFreqs (f0);

        const float blurCoef = s.blur * 0.985f;
        float maxMag = 0.0f;
        for (size_t k = 1; k < (size_t) bins; ++k)
        {
            const float m = m0[k] + frac * (m1[k] - m0[k]);
            blurMag[k] = blurMag[k] * blurCoef + m * (1.0f - blurCoef);
            maxMag = std::max (maxMag, blurMag[k]);
        }

        std::fill (targetMag.begin(), targetMag.end(), 0.0f);
        const float threshold = maxMag * s.sparse * s.sparse * 0.35f;
        const float binPerHz = (float) (N / hostRate);

        for (size_t k = 1; k < (size_t) bins; ++k)
        {
            const float m = blurMag[k];
            if (m <= threshold)
                continue;

            const float f = q0[k] * ratio + s.shiftHz;
            const int t = (int) (f * binPerHz + 0.5f);
            if (t < 1 || t >= bins - 1)
                continue;

            if (m > targetMag[(size_t) t])
            {
                targetMag[(size_t) t] = m;
                targetFreq[(size_t) t] = f;
            }
        }

        std::fill (fftData.begin(), fftData.end(), 0.0f);
        const float phasePerHz = juce::MathConstants<float>::twoPi * (float) H / (float) hostRate;
        const float twoPi = juce::MathConstants<float>::twoPi;

        for (size_t t = 1; t < (size_t) bins - 1; ++t)
        {
            const float m = targetMag[t];
            if (m <= 0.0f)
                continue;

            float p = phase[t] + targetFreq[t] * phasePerHz;
            if (s.whisper > 0.0f)
                p += s.whisper * juce::MathConstants<float>::pi * rng.bipolar();
            p -= twoPi * std::floor (p / twoPi);
            phase[t] = p;

            fftData[t * 2]     = m * std::cos (p);
            fftData[t * 2 + 1] = m * std::sin (p);
        }

        fft->performRealOnlyInverseTransform (fftData.data());

        for (size_t i = 0; i < (size_t) N; ++i)
            ola[i] += fftData[i] * window[i];

        std::copy (ola.begin(), ola.begin() + H, outBuf.begin());
        std::copy (ola.begin() + H, ola.end(), ola.begin());
        std::fill (ola.end() - H, ola.end(), 0.0f);
    }

    //==============================================================================
    double hostRate = 44100.0;
    const juce::dsp::FFT* fft = nullptr;
    fx::Random rng;

    double playPos = 0.0;
    bool finished = false;

    std::array<Grain, 24> grains {};
    double scanPos = 0.0;
    float spawnTimer = 0.0f;

    double specPos = 0.0;
    int outRead = H;
    std::vector<float> phase, blurMag, targetMag, targetFreq, ola, fftData, outBuf, window;
};
