#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <string_view>
#include <unordered_map>
#include "Parameters.h"
#include "SynthEngine.h"
#include "Effects.h"
#include "SampleData.h"

// Кольцевой буфер «аудио → интерфейс» для осциллографа и спектрограммы.
// Аудиопоток только пишет, интерфейс только читает, без блокировок.
struct AudioTap
{
    static constexpr int size = 1 << 14;
    juce::AbstractFifo fifo { size };
    std::vector<float> left = std::vector<float> (size), right = std::vector<float> (size);

    void push (const float* l, const float* r, int n) noexcept
    {
        const auto scope = fifo.write (n);
        std::copy (l, l + scope.blockSize1, left.data() + scope.startIndex1);
        std::copy (r, r + scope.blockSize1, right.data() + scope.startIndex1);
        std::copy (l + scope.blockSize1, l + scope.blockSize1 + scope.blockSize2, left.data() + scope.startIndex2);
        std::copy (r + scope.blockSize1, r + scope.blockSize1 + scope.blockSize2, right.data() + scope.startIndex2);
    }

    int pull (float* l, float* r, int maxSamples) noexcept
    {
        const auto scope = fifo.read (std::min (maxSamples, fifo.getNumReady()));
        std::copy (left.data() + scope.startIndex1, left.data() + scope.startIndex1 + scope.blockSize1, l);
        std::copy (right.data() + scope.startIndex1, right.data() + scope.startIndex1 + scope.blockSize1, r);
        std::copy (left.data() + scope.startIndex2, left.data() + scope.startIndex2 + scope.blockSize2, l + scope.blockSize1);
        std::copy (right.data() + scope.startIndex2, right.data() + scope.startIndex2 + scope.blockSize2, r + scope.blockSize1);
        return scope.blockSize1 + scope.blockSize2;
    }
};

class KozyavkaProcessor : public juce::AudioProcessor,
                          private juce::AsyncUpdater
{
public:
    KozyavkaProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                  { return true; }

    const juce::String getName() const override      { return JucePlugin_Name; }
    bool acceptsMidi() const override                { return true; }
    bool producesMidi() const override               { return false; }
    bool isMidiEffect() const override               { return false; }
    double getTailLengthSeconds() const override     { return 0.0; }

    // Пресеты видны во FL как программы: 0 — init, дальше заводские
    int getNumPrograms() override;
    int getCurrentProgram() override                 { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    enum class Slot { sample, texture };

    // Только из потока интерфейса
    bool loadSampleFile (const juce::File& file, Slot slot, juce::String& error);
    const SampleData* getCurrentSample() const;     // по выбранному источнику
    const SampleData* getTextureSample() const;

    juce::AudioProcessorValueTreeState apvts;
    AudioTap tap;
    SampleVisualFeed visualFeed;
    std::atomic<float> guiActivity { 0.0f };
    int editorPage = 0;          // последняя открытая вкладка
    bool visualsEnabled = true;  // анимации можно выключить на слабом компьютере

private:
    void applyPreset (int index);
    void handleAsyncUpdate() override  { applyPreset (pendingProgram.load()); }
    float value (std::string_view id) const noexcept;
    SynthSettings readSettings() const;
    FxSettings readFxSettings() const;
    Transport readTransport();

    std::unordered_map<std::string_view, std::atomic<float>*> rawParams;
    std::array<std::array<std::atomic<float>*, 5>, Params::numOscillators> oscRaw {};   // wave, coarse, fine, pan, level
    SynthEngine engine;
    FxChain effects;
    float lastMaster = 0.7f;
    std::atomic<int> currentProgram { 0 }, pendingProgram { 0 };

    // Загруженные пользователем сэмплы. Старые удаляются, только когда аудиопоток
    // гарантированно их больше не читает (прошло хотя бы два аудиоблока).
    std::vector<std::unique_ptr<SampleData>> ownedSamples;
    struct Retired { std::unique_ptr<SampleData> data; uint32_t stamp; };
    std::vector<Retired> retired;
    std::atomic<const SampleData*> userSample { nullptr }, userTexture { nullptr };
    std::atomic<uint32_t> blockCounter { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KozyavkaProcessor)
};
