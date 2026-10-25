#pragma once

#include "PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>

// Общая палитра интерфейса
namespace Theme
{
    const juce::Colour bgTop     { 0xff05060a };
    const juce::Colour bgBottom  { 0xff0f121b };
    const juce::Colour panel     { 0xff0a0d14 };
    const juce::Colour knobDark  { 0xff0a0d14 };
    const juce::Colour knobLight { 0xff3c4859 };
    const juce::Colour tick      { 0xffdfe7f2 };
    const juce::Colour text      { 0xffc9d2de };
    const juce::Colour textDim   { 0xff5d6878 };
    const juce::Colour cyan      { 0xff46e6ff };
    const juce::Colour lime      { 0xffc6ff3d };
    const juce::Colour mint      { 0xff5cf2b8 };
    const juce::Colour violet    { 0xffb27cff };
    const juce::Colour pink      { 0xffff5fa2 };
}

juce::Font sansFont (float height, bool bold = false);
juce::Font monoFont (float height);
juce::Colour heatColour (float value);   // 0..1 → цвет спектрограммы (тёмно-фиолетовый → розовый → белый)

//==============================================================================
// Заголовок, который иногда «глючит»: RGB-расслоение и сдвиг полос, как битый VHS
class GlitchTitle : public juce::Component
{
public:
    void setAccent (juce::Colour c) { accent = c; renderText(); repaint(); }
    void tick();
    void paint (juce::Graphics&) override;
    void resized() override { renderText(); }

private:
    void renderText();

    juce::Image textImage;
    juce::Colour accent { Theme::cyan };
    juce::Random rng;
    int glitchFrames = 0;
    float split = 0.0f;
    std::array<int, 6> sliceOffsets {};
};

//==============================================================================
// Анимированная форма суммарной волны трёх осцилляторов со «шлейфом»
class WaveMorphView : public juce::Component
{
public:
    explicit WaveMorphView (KozyavkaProcessor&);

    void setAccent (juce::Colour c) { accent = c; repaint(); }
    void tick();
    void paint (juce::Graphics&) override;

private:
    float waveValue (int wave, float phase);

    KozyavkaProcessor& processor;
    std::array<std::array<std::atomic<float>*, 4>, 3> osc {};   // wave, coarse, fine, level
    std::atomic<float>* smpLevel = nullptr;
    juce::Colour accent { Theme::cyan };
    juce::Random rng;
    float time = 0.0f, activity = 0.0f, blink = 0.0f;
};

//==============================================================================
// Анализатор выхода: waterfall (звук превращается в картинку), спектр, осциллограф, лиссажу
class AnalyzerView : public juce::Component
{
public:
    explicit AnalyzerView (KozyavkaProcessor&);

    void setAccent (juce::Colour c) { accent = c; }
    void tick (bool visible);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    enum Mode { waterfall, spectrum, scope, lissajous, numModes };

    void computeSpectrum();
    void drawWaterfallColumns (int columns);
    void drawPhosphor();
    float magnitudeAt (float hz) const;

    KozyavkaProcessor& processor;
    static constexpr int fftOrder = 11, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> historyL = std::vector<float> (fftSize), historyR = std::vector<float> (fftSize);
    std::vector<float> incomingL = std::vector<float> (AudioTap::size), incomingR = std::vector<float> (AudioTap::size);
    std::vector<float> fftData = std::vector<float> (fftSize * 2), magnitudes = std::vector<float> (fftSize / 2 + 1);
    std::vector<float> smoothed, peaks;
    juce::Image waterfallImage, phosphorImage;
    int mode = waterfall;
    juce::Colour accent { Theme::cyan };
};

//==============================================================================
// Вид сэмпла: волна + спектрограмма, бегущие головки голосов и разлетающиеся гранулы
class SampleView : public juce::Component
{
public:
    explicit SampleView (KozyavkaProcessor&);

    std::function<void()> onLoadRequest;
    void setAccent (juce::Colour c) { accent = c; }
    void tick();
    void paint (juce::Graphics&) override;
    void resized() override { cached = nullptr; }
    void mouseUp (const juce::MouseEvent&) override;

private:
    void rebuild (const SampleData&);

    struct Particle { float x, y, vy, age; };

    KozyavkaProcessor& processor;
    const SampleData* cached = nullptr;
    juce::Image waveImage, spectrogramImage;
    std::vector<Particle> particles;
    uint32_t lastGrain = 0;
    juce::Random rng;
    juce::Colour accent { Theme::lime };
};
