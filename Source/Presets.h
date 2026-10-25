#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

// Заводские пресеты. Значения указаны в «настоящих» единицах параметров
// (секунды, герцы, индексы вариантов), всё неуказанное берётся по умолчанию.
namespace Presets
{
    struct Preset
    {
        const char* category;
        const char* name;
        std::vector<std::pair<const char*, float>> values;
    };

    inline const juce::StringArray categories { "Bass", "Lead", "Pad", "Pluck", "Atmosphere", "Spectral", "Granular" };

    const std::vector<Preset>& all();
}
