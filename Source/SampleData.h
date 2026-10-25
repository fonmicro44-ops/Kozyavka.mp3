#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <memory>
#include <vector>

// Звук для сэмплера и текстур: моно-аудио + заранее посчитанный спектральный анализ
// (STFT). Анализ делается один раз при загрузке, поэтому спектральный режим
// во время игры только «собирает» звук обратно и не анализирует его заново.
struct SampleData
{
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;   // 2048
    static constexpr int hop      = fftSize / 4;     // 512
    static constexpr int numBins  = fftSize / 2 + 1;

    juce::String name;
    std::vector<float> audio;
    double sampleRate = 44100.0;
    int rootNote = 60;            // C4: на этой ноте сэмпл звучит в своей высоте

    int numFrames = 0;
    std::vector<float> mags;      // numFrames × numBins: амплитуды
    std::vector<float> freqs;     // numFrames × numBins: точные частоты (Гц) по фазовому вокодеру

    int length() const noexcept { return (int) audio.size(); }
    const float* frameMags (int frame) const noexcept  { return mags.data()  + (size_t) frame * numBins; }
    const float* frameFreqs (int frame) const noexcept { return freqs.data() + (size_t) frame * numBins; }

    // Чтение с линейной интерполяцией и заворачиванием по кругу
    float readWrapped (double pos) const noexcept
    {
        const int len = length();
        pos -= std::floor (pos / len) * len;
        const int i = (int) pos;
        const float frac = (float) (pos - i);
        const float a = audio[(size_t) i];
        const float b = audio[(size_t) (i + 1 < len ? i + 1 : 0)];
        return a + frac * (b - a);
    }

    void analyse();
    void finalise();   // нормализация, микро-фейды, анализ

    static std::unique_ptr<SampleData> fromFile (const juce::File& file, juce::String& error);
};

// Пять встроенных сэмплов, синтезированных кодом (файлы не нужны)
struct BuiltinSamples
{
    static constexpr int count = 5;   // Glass, Choir, Metal, Breath, Vox
    std::array<std::unique_ptr<SampleData>, count> items;

    static const BuiltinSamples& get();
};
