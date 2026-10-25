#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Все параметры плагина. Их видит FL Studio: их можно автоматизировать
// и привязывать к MIDI-контроллеру.
namespace Params
{
    inline const juce::StringArray waveNames         { "Sine", "Triangle", "Saw", "Square", "Noise" };
    inline const juce::StringArray lfoTargetNames    { "Pitch", "Filter", "Volume" };
    inline const juce::StringArray harmonyNames      { "Off", "Octave", "Fifth", "Power", "Major", "Minor", "Sus2", "Minor7", "Sub" };
    inline const juce::StringArray textureNames      { "Wind", "Vinyl", "Rain", "Static", "Ocean", "Fire", "Sample" };
    inline const juce::StringArray textureFollowNames { "Notes", "Always" };
    inline const juce::StringArray driveModeNames    { "Warm", "Fold", "Fuzz" };
    inline const juce::StringArray sampleModeNames   { "Classic", "Granular", "Spectral" };
    inline const juce::StringArray sampleSourceNames { "Glass", "Choir", "Metal", "Breath", "Vox", "User" };
    inline const juce::StringArray onOffNames        { "Off", "On" };
    inline const juce::StringArray motionRateNames   { "1/4", "1/8", "1/16", "1/32" };
    inline const juce::StringArray motionShapeNames  { "Smooth", "Gate", "Pump" };
    inline const juce::StringArray crystalPitchNames { "Octave Up", "Fifth Up", "Octave Down", "Reverse" };
    inline const juce::StringArray spaceColorNames   { "Bright", "Dark" };
    inline const juce::StringArray freezeNames       { "Off", "On" };

    constexpr int numOscillators = 3;

    // ID параметра осциллятора: osc1_wave, osc2_coarse ...
    inline juce::String osc (int index, const char* name)
    {
        return "osc" + juce::String (index + 1) + "_" + name;
    }

    constexpr auto attack          = "attack";
    constexpr auto decay           = "decay";
    constexpr auto sustain         = "sustain";
    constexpr auto release         = "release";
    constexpr auto cutoff          = "cutoff";
    constexpr auto resonance       = "resonance";
    constexpr auto filterEnv       = "filter_env";
    constexpr auto lfoRate         = "lfo_rate";
    constexpr auto lfoAmount       = "lfo_amount";
    constexpr auto lfoTarget       = "lfo_target";
    constexpr auto glide           = "glide";
    constexpr auto voices          = "voices";
    constexpr auto master          = "master";
    constexpr auto harmony         = "harmony";
    constexpr auto textureType     = "texture_type";
    constexpr auto textureAmount   = "texture_amount";
    constexpr auto drive           = "drive";
    constexpr auto driveMode       = "drive_mode";
    constexpr auto vowel           = "vowel";
    constexpr auto vowelMix        = "vowel_mix";
    constexpr auto motionDepth     = "motion_depth";
    constexpr auto motionRate      = "motion_rate";
    constexpr auto motionShape     = "motion_shape";
    constexpr auto crystalMix      = "crystal_mix";
    constexpr auto crystalFeedback = "crystal_feedback";
    constexpr auto crystalPitch    = "crystal_pitch";
    constexpr auto spaceMix        = "space_mix";
    constexpr auto spaceSize       = "space_size";
    constexpr auto shimmer         = "shimmer";
    constexpr auto spaceColor      = "space_color";
    constexpr auto freeze          = "freeze";
    constexpr auto spacePredelay   = "space_predelay";

    constexpr auto textureTone      = "texture_tone";
    constexpr auto textureDensity   = "texture_density";
    constexpr auto textureVariation = "texture_variation";
    constexpr auto textureWidth     = "texture_width";
    constexpr auto textureSeed      = "texture_seed";
    constexpr auto textureFollow    = "texture_follow";

    constexpr auto crushBits = "crush_bits";
    constexpr auto crushRate = "crush_rate";
    constexpr auto crushMix  = "crush_mix";

    constexpr auto smpLevel    = "smp_level";
    constexpr auto smpCoarse   = "smp_coarse";
    constexpr auto smpFine     = "smp_fine";
    constexpr auto smpPan      = "smp_pan";
    constexpr auto smpMode     = "smp_mode";
    constexpr auto smpSource   = "smp_source";
    constexpr auto smpPosition = "smp_position";
    constexpr auto smpScan     = "smp_scan";
    constexpr auto smpLoop     = "smp_loop";
    constexpr auto smpKeytrack = "smp_keytrack";
    constexpr auto smpGrain    = "smp_grain";
    constexpr auto smpDensity  = "smp_density";
    constexpr auto smpSpray    = "smp_spray";
    constexpr auto smpJitter   = "smp_jitter";
    constexpr auto smpReverse  = "smp_reverse";
    constexpr auto smpBlur     = "smp_blur";
    constexpr auto smpShift    = "smp_shift";
    constexpr auto smpSparse   = "smp_sparse";
    constexpr auto smpWhisper  = "smp_whisper";

    // Сдвиг спектра: квадратичная шкала, чтобы около нуля можно было крутить точно
    inline float shiftToHz (float v) { return (v < 0.0f ? -1.0f : 1.0f) * v * v * 1000.0f; }

    // Как значения выглядят в интерфейсе и в FL Studio
    namespace Format
    {
        inline juce::String percent (float v, int)  { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
        inline juce::String signedPercent (float v, int)
        {
            const int p = juce::roundToInt (v * 100.0f);
            return (p > 0 ? "+" : "") + juce::String (p) + "%";
        }
        inline juce::String time (float v, int)
        {
            return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s";
        }
        inline juce::String hertz (float v, int)
        {
            return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz" : juce::String (v / 1000.0f, 1) + " kHz";
        }
        inline juce::String rate (float v, int)     { return juce::String (v, 2) + " Hz"; }
        inline juce::String cents (float v, int)
        {
            const int c = juce::roundToInt (v);
            return (c > 0 ? "+" : "") + juce::String (c) + " ct";
        }
        inline juce::String pan (float v, int)
        {
            const int p = juce::roundToInt (v * 100.0f);
            return p == 0 ? juce::String ("C") : (p < 0 ? "L" + juce::String (-p) : "R" + juce::String (p));
        }
        inline juce::String semitones (int v, int)  { return (v > 0 ? "+" : "") + juce::String (v) + " st"; }
        inline juce::String speed (float v, int)    { return "x" + juce::String (v, 2); }
        inline juce::String perSecond (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " /s"; }
        inline juce::String bits (float v, int)     { return juce::String (v, 1) + " bit"; }
        inline juce::String shift (float v, int)
        {
            const int hz = juce::roundToInt (shiftToHz (v));
            return (hz > 0 ? "+" : "") + juce::String (hz) + " Hz";
        }
        inline juce::String vowel (float v, int)
        {
            static const char* names[] { "a", "e", "i", "o", "u" };
            return juce::String (names[juce::jlimit (0, 4, juce::roundToInt (v * 4.0f))]);
        }
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        using Formatter = std::function<String (float, int)>;

        AudioProcessorValueTreeState::ParameterLayout layout;

        auto skewed = [] (float min, float max, float centre)
        {
            NormalisableRange<float> range (min, max);
            range.setSkewForCentre (centre);
            return range;
        };

        auto addFloat = [&] (const String& id, const String& name, NormalisableRange<float> range,
                             float defaultValue, Formatter format)
        {
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, defaultValue,
                                                               AudioParameterFloatAttributes().withStringFromValueFunction (std::move (format))));
        };

        auto addChoice = [&] (const String& id, const String& name, const StringArray& items, int defaultIndex)
        {
            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, defaultIndex));
        };

        // Значения по умолчанию: пила + квадрат октавой ниже с лёгкой расстройкой
        const int   defaultWave[]   { 2, 3, 0 };
        const int   defaultCoarse[] { 0, -12, 0 };
        const float defaultFine[]   { 0.0f, 7.0f, 0.0f };
        const float defaultLevel[]  { 1.0f, 0.5f, 0.0f };

        for (int i = 0; i < numOscillators; ++i)
        {
            const auto name = "Osc " + String (i + 1) + " ";

            addChoice (osc (i, "wave"), name + "Wave", waveNames, defaultWave[i]);
            layout.add (std::make_unique<AudioParameterInt> (ParameterID { osc (i, "coarse"), 1 }, name + "Coarse",
                                                             -24, 24, defaultCoarse[i],
                                                             AudioParameterIntAttributes().withStringFromValueFunction (Format::semitones)));
            addFloat (osc (i, "fine"),  name + "Fine",  { -100.0f, 100.0f }, defaultFine[i],  Format::cents);
            addFloat (osc (i, "pan"),   name + "Pan",   { -1.0f, 1.0f },     0.0f,            Format::pan);
            addFloat (osc (i, "level"), name + "Level", { 0.0f, 1.0f },      defaultLevel[i], Format::percent);
        }

        addFloat (attack,  "Attack",  skewed (0.001f, 5.0f, 0.3f),  0.005f, Format::time);
        addFloat (decay,   "Decay",   skewed (0.001f, 5.0f, 0.5f),  0.3f,   Format::time);
        addFloat (sustain, "Sustain", { 0.0f, 1.0f },               0.7f,   Format::percent);
        addFloat (release, "Release", skewed (0.001f, 10.0f, 0.8f), 0.3f,   Format::time);

        addFloat (cutoff,    "Cutoff",     skewed (20.0f, 20000.0f, 1000.0f), 20000.0f, Format::hertz);
        addFloat (resonance, "Resonance",  { 0.0f, 1.0f },  0.1f, Format::percent);
        addFloat (filterEnv, "Filter Env", { -1.0f, 1.0f }, 0.0f, Format::signedPercent);

        addFloat (lfoRate,   "LFO Rate",   skewed (0.05f, 20.0f, 3.0f), 5.0f, Format::rate);
        addFloat (lfoAmount, "LFO Amount", { 0.0f, 1.0f }, 0.0f, Format::percent);
        addChoice (lfoTarget, "LFO Target", lfoTargetNames, 0);

        addFloat (glide, "Glide", skewed (0.0f, 1.0f, 0.15f), 0.0f, Format::time);
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { voices, 1 }, "Voices", 1, 16, 8));
        addFloat (master, "Master", { 0.0f, 1.0f }, 0.7f, Format::percent);

        // Звуковой дизайн
        addChoice (harmony, "Harmony", harmonyNames, 0);

        addChoice (textureType,      "Texture Type",      textureNames, 0);
        addFloat (textureAmount,     "Texture Amount",    { 0.0f, 1.0f }, 0.0f, Format::percent);
        addFloat (textureTone,       "Texture Tone",      { 0.0f, 1.0f }, 0.6f, Format::percent);
        addFloat (textureDensity,    "Texture Density",   { 0.0f, 1.0f }, 0.5f, Format::percent);
        addFloat (textureVariation,  "Texture Variation", { 0.0f, 1.0f }, 0.5f, Format::percent);
        addFloat (textureWidth,      "Texture Width",     { 0.0f, 1.0f }, 0.8f, Format::percent);
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { textureSeed, 1 }, "Texture Seed", 0, 999, 1));
        addChoice (textureFollow,    "Texture Follow",    textureFollowNames, 0);

        addFloat (crushBits, "Crush Bits", { 1.0f, 16.0f }, 8.0f, Format::bits);
        addFloat (crushRate, "Crush Rate", skewed (200.0f, 48000.0f, 6000.0f), 12000.0f, Format::hertz);
        addFloat (crushMix,  "Crush Mix",  { 0.0f, 1.0f }, 0.0f, Format::percent);

        // Сэмплер
        addFloat (smpLevel, "Sample Level", { 0.0f, 1.0f }, 0.0f, Format::percent);
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { smpCoarse, 1 }, "Sample Coarse", -24, 24, 0,
                                                         AudioParameterIntAttributes().withStringFromValueFunction (Format::semitones)));
        addFloat (smpFine, "Sample Fine", { -100.0f, 100.0f }, 0.0f, Format::cents);
        addFloat (smpPan,  "Sample Pan",  { -1.0f, 1.0f },     0.0f, Format::pan);
        addChoice (smpMode,   "Sample Mode",   sampleModeNames, 1);
        addChoice (smpSource, "Sample Source", sampleSourceNames, 1);
        addFloat (smpPosition, "Sample Position", { 0.0f, 1.0f },  0.0f, Format::percent);
        addFloat (smpScan,     "Sample Scan",     { -2.0f, 2.0f }, 1.0f, Format::speed);
        addChoice (smpLoop,     "Sample Loop",     onOffNames, 1);
        addChoice (smpKeytrack, "Sample Keytrack", onOffNames, 1);
        addFloat (smpGrain,   "Grain Size",    skewed (0.01f, 0.5f, 0.1f), 0.12f, Format::time);
        addFloat (smpDensity, "Grain Density", skewed (1.0f, 100.0f, 15.0f), 20.0f, Format::perSecond);
        addFloat (smpSpray,   "Grain Spray",   { 0.0f, 1.0f }, 0.1f, Format::percent);
        addFloat (smpJitter,  "Grain Jitter",  { 0.0f, 1.0f }, 0.0f, Format::percent);
        addFloat (smpReverse, "Grain Reverse", { 0.0f, 1.0f }, 0.0f, Format::percent);
        addFloat (smpBlur,    "Spectral Blur",    { 0.0f, 1.0f },  0.0f, Format::percent);
        addFloat (smpShift,   "Spectral Shift",   { -1.0f, 1.0f }, 0.0f, Format::shift);
        addFloat (smpSparse,  "Spectral Sparse",  { 0.0f, 1.0f },  0.0f, Format::percent);
        addFloat (smpWhisper, "Spectral Whisper", { 0.0f, 1.0f },  0.0f, Format::percent);

        addFloat (drive, "Drive", { 0.0f, 1.0f }, 0.0f, Format::percent);
        addChoice (driveMode, "Drive Mode", driveModeNames, 0);

        addFloat (vowel,    "Vowel",     { 0.0f, 1.0f }, 0.0f, Format::vowel);
        addFloat (vowelMix, "Vowel Mix", { 0.0f, 1.0f }, 0.0f, Format::percent);

        addFloat (motionDepth, "Motion Depth", { 0.0f, 1.0f }, 0.0f, Format::percent);
        addChoice (motionRate,  "Motion Rate",  motionRateNames, 2);
        addChoice (motionShape, "Motion Shape", motionShapeNames, 1);

        addFloat (crystalMix,      "Crystal Mix",      { 0.0f, 1.0f },  0.0f, Format::percent);
        addFloat (crystalFeedback, "Crystal Feedback", { 0.0f, 0.95f }, 0.4f, Format::percent);
        addChoice (crystalPitch, "Crystal Pitch", crystalPitchNames, 0);

        addFloat (spaceMix,  "Space Mix",  { 0.0f, 1.0f }, 0.0f, Format::percent);
        addFloat (spaceSize, "Space Size", { 0.0f, 1.0f }, 0.6f, Format::percent);
        addFloat (shimmer,   "Shimmer",    { 0.0f, 1.0f }, 0.3f, Format::percent);
        addChoice (spaceColor, "Space Color", spaceColorNames, 0);
        addChoice (freeze,     "Freeze",      freezeNames, 0);
        addFloat (spacePredelay, "Space Predelay", { 0.0f, 0.3f }, 0.02f, Format::time);

        return layout;
    }
}
