#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// Общие маленькие DSP-кирпичики для синтезатора и эффектов
namespace fx
{
    //==============================================================================
    struct Random
    {
        uint32_t state = 0x1234567u;

        float bipolar() noexcept   // -1..1
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return (float) (int32_t) state * (1.0f / 2147483648.0f);
        }

        float unipolar() noexcept { return bipolar() * 0.5f + 0.5f; }   // 0..1

        void seed (uint32_t s) noexcept { state = s * 2654435761u + 0x9e3779b9u; if (state == 0) state = 1; }
    };

    inline float softClip (float x) noexcept
    {
        x = juce::jlimit (-3.0f, 3.0f, x);
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

    // Окно Ханна из таблицы: два окна со сдвигом в половину в сумме дают ровно 1
    inline float hann (float p) noexcept
    {
        static const auto table = []
        {
            std::array<float, 1025> t {};
            for (size_t i = 0; i < t.size(); ++i)
            {
                const double s = std::sin (juce::MathConstants<double>::pi * (double) i / 1024.0);
                t[i] = (float) (s * s);
            }
            return t;
        }();

        const float x = p * 1024.0f;
        const int i = juce::jlimit (0, 1023, (int) x);
        return table[(size_t) i] + (x - (float) i) * (table[(size_t) i + 1] - table[(size_t) i]);
    }

    //==============================================================================
    // Фильтр TPT SVF: полосовой (с единичным усилением на резонансе) и ФНЧ
    struct Svf
    {
        float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, k = 1.0f;
        float ic1 = 0.0f, ic2 = 0.0f;

        void set (float hz, float q, float sampleRate) noexcept
        {
            hz = juce::jlimit (20.0f, sampleRate * 0.45f, hz);
            const float g = std::tan (juce::MathConstants<float>::pi * hz / sampleRate);
            k = 1.0f / q;
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        void reset() noexcept { ic1 = ic2 = 0.0f; }

        float bandpass (float x) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return k * v1;
        }

        float lowpass (float x) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return v2;
        }
    };

    // Простейший однополюсный ФНЧ: дёшево сглаживает параметры и шум
    struct OnePole
    {
        float coef = 1.0f, state = 0.0f;

        void setCutoff (float hz, float sampleRate) noexcept
        {
            coef = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / sampleRate);
        }

        float process (float x) noexcept { return state += (x - state) * coef; }
    };

    //==============================================================================
    // Кольцевой буфер задержки (размер — степень двойки, чтобы обходиться без деления)
    struct DelayBuffer
    {
        std::vector<float> data;
        int mask = 0, write = 0;

        void setMinimumSize (int samples)
        {
            data.assign ((size_t) juce::nextPowerOfTwo (samples + 2), 0.0f);
            mask = (int) data.size() - 1;
            write = 0;
        }

        void clear() noexcept { std::fill (data.begin(), data.end(), 0.0f); }

        void push (float x) noexcept
        {
            data[(size_t) write] = x;
            write = (write + 1) & mask;
        }

        float at (int samplesAgo) const noexcept { return data[(size_t) ((write - samplesAgo) & mask)]; }

        float atFractional (float samplesAgo) const noexcept
        {
            const int whole = (int) samplesAgo;
            const float frac = samplesAgo - (float) whole;
            const float a = at (whole), b = at (whole + 1);
            return a + frac * (b - a);
        }
    };
}
