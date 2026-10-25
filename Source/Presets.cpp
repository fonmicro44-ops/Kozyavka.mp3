#include "Presets.h"

// Подсказка по индексам:
//   волны: 0 sine, 1 triangle, 2 saw, 3 square, 4 noise
//   lfo_target: 0 pitch, 1 filter, 2 volume      drive_mode: 0 warm, 1 fold, 2 fuzz
//   harmony: 0 off, 1 octave, 2 fifth, 3 power, 4 major, 5 minor, 6 sus2, 7 minor7, 8 sub
//   smp_mode: 0 classic, 1 granular, 2 spectral   smp_source: 0 glass, 1 choir, 2 metal, 3 breath, 4 vox
//   texture_type: 0 wind, 1 vinyl, 2 rain, 3 static, 4 ocean, 5 fire, 6 sample   texture_follow: 0 notes, 1 always
//   motion_rate: 0 1/4, 1 1/8, 2 1/16, 3 1/32     motion_shape: 0 smooth, 1 gate, 2 pump
//   crystal_pitch: 0 octave up, 1 fifth up, 2 octave down, 3 reverse     space_color: 0 bright, 1 dark

const std::vector<Presets::Preset>& Presets::all()
{
    static const std::vector<Preset> presets
    {
        //==============================================================================
        // BASS — гиперпоп 2019–2022: перегруженные 808, резиновые квадраты, биткраш и сайдчейн-«насос»
        { "Bass", "gecgec 808", {
            { "osc1_wave", 0 }, { "osc1_level", 1.0f },
            { "osc2_wave", 1 }, { "osc2_coarse", 12 }, { "osc2_fine", 0 }, { "osc2_level", 0.25f },
            { "attack", 0.002f }, { "decay", 1.4f }, { "sustain", 0.35f }, { "release", 0.25f },
            { "cutoff", 4500 }, { "resonance", 0.15f },
            { "glide", 0.09f }, { "voices", 1 },
            { "drive", 0.75f }, { "drive_mode", 0 },
            { "crush_mix", 0.25f }, { "crush_bits", 9 }, { "crush_rate", 9000 },
            { "master", 0.6f } } },

        { "Bass", "rubber sophie", {
            { "osc1_wave", 3 }, { "osc1_level", 1.0f },
            { "osc2_wave", 3 }, { "osc2_coarse", 0 }, { "osc2_fine", 9 }, { "osc2_level", 0.8f },
            { "osc3_wave", 0 }, { "osc3_coarse", -12 }, { "osc3_level", 0.6f },
            { "attack", 0.003f }, { "decay", 0.28f }, { "sustain", 0.25f }, { "release", 0.12f },
            { "cutoff", 380 }, { "resonance", 0.62f }, { "filter_env", 0.65f },
            { "drive", 0.5f }, { "drive_mode", 1 },
            { "glide", 0.045f }, { "voices", 1 }, { "master", 0.7f } } },

        { "Bass", "bitglass sub", {
            { "osc1_wave", 0 }, { "osc1_level", 1.0f },
            { "osc2_wave", 2 }, { "osc2_coarse", 0 }, { "osc2_fine", -6 }, { "osc2_level", 0.35f },
            { "osc3_wave", 3 }, { "osc3_coarse", 12 }, { "osc3_level", 0.15f },
            { "attack", 0.004f }, { "decay", 0.6f }, { "sustain", 0.7f }, { "release", 0.2f },
            { "cutoff", 1800 }, { "resonance", 0.3f }, { "filter_env", 0.3f },
            { "harmony", 8 }, { "voices", 2 }, { "glide", 0.06f },
            { "drive", 0.45f }, { "drive_mode", 2 },
            { "crush_mix", 0.45f }, { "crush_bits", 6 }, { "crush_rate", 5500 },
            { "motion_depth", 0.45f }, { "motion_rate", 2 }, { "motion_shape", 2 },
            { "smp_level", 0.2f }, { "smp_mode", 0 }, { "smp_source", 0 }, { "smp_coarse", 12 }, { "smp_loop", 0 },
            { "master", 0.55f } } },

        //==============================================================================
        // LEAD — эйсид в духе Drukqs, «цветочный» вокальный лид, рваный Windowlicker
        { "Lead", "drukqs acid", {
            { "osc1_wave", 2 }, { "osc1_level", 1.0f }, { "osc2_level", 0.0f },
            { "attack", 0.002f }, { "decay", 0.22f }, { "sustain", 0.12f }, { "release", 0.15f },
            { "cutoff", 420 }, { "resonance", 0.82f }, { "filter_env", 0.7f },
            { "drive", 0.45f }, { "drive_mode", 0 },
            { "glide", 0.07f }, { "voices", 1 },
            { "crystal_mix", 0.12f }, { "crystal_feedback", 0.3f },
            { "space_mix", 0.15f }, { "space_size", 0.4f }, { "master", 0.9f } } },

        { "Lead", "petal vox", {
            { "osc1_wave", 2 }, { "osc1_fine", -8 }, { "osc1_level", 0.8f },
            { "osc2_wave", 3 }, { "osc2_coarse", 0 }, { "osc2_fine", 8 }, { "osc2_level", 0.6f },
            { "osc3_wave", 0 }, { "osc3_coarse", 12 }, { "osc3_level", 0.3f },
            { "attack", 0.04f }, { "decay", 0.5f }, { "sustain", 0.7f }, { "release", 0.4f },
            { "cutoff", 5200 }, { "resonance", 0.2f },
            { "vowel", 0.3f }, { "vowel_mix", 0.45f }, { "harmony", 1 },
            { "lfo_rate", 5.5f }, { "lfo_amount", 0.06f }, { "lfo_target", 0 },
            { "crystal_mix", 0.28f }, { "crystal_feedback", 0.45f },
            { "space_mix", 0.3f }, { "space_size", 0.55f }, { "shimmer", 0.45f }, { "master", 0.55f } } },

        { "Lead", "windowlicker", {
            { "osc1_wave", 2 }, { "osc1_level", 1.0f },
            { "osc2_wave", 2 }, { "osc2_coarse", 12 }, { "osc2_fine", 14 }, { "osc2_level", 0.5f },
            { "osc3_wave", 4 }, { "osc3_level", 0.05f },
            { "attack", 0.005f }, { "decay", 0.35f }, { "sustain", 0.5f }, { "release", 0.2f },
            { "cutoff", 2600 }, { "resonance", 0.45f }, { "filter_env", 0.35f },
            { "drive", 0.3f }, { "drive_mode", 1 },
            { "crush_mix", 0.3f }, { "crush_bits", 11 }, { "crush_rate", 16000 },
            { "motion_depth", 0.3f }, { "motion_rate", 3 }, { "motion_shape", 1 },
            { "glide", 0.03f }, { "voices", 1 },
            { "space_mix", 0.2f }, { "space_size", 0.5f }, { "space_color", 1 }, { "master", 0.72f } } },

        //==============================================================================
        // PAD — ночной Burial, тягучий witch house Salem, «цветущая» botanica
        { "Pad", "night bus", {
            { "smp_level", 0.7f }, { "smp_mode", 1 }, { "smp_source", 1 }, { "smp_position", 0.2f }, { "smp_scan", 0.08f },
            { "smp_grain", 0.25f }, { "smp_density", 24 }, { "smp_spray", 0.35f }, { "smp_reverse", 0.15f },
            { "osc1_wave", 2 }, { "osc1_fine", -10 }, { "osc1_level", 0.25f },
            { "osc2_wave", 2 }, { "osc2_coarse", 0 }, { "osc2_fine", 10 }, { "osc2_level", 0.25f },
            { "attack", 1.2f }, { "decay", 1.0f }, { "sustain", 0.8f }, { "release", 2.8f },
            { "cutoff", 2400 }, { "resonance", 0.15f },
            { "texture_type", 1 }, { "texture_amount", 0.45f }, { "texture_tone", 0.45f }, { "texture_density", 0.4f },
            { "texture_variation", 0.6f }, { "texture_seed", 7 },
            { "crush_mix", 0.2f }, { "crush_bits", 12 }, { "crush_rate", 14000 },
            { "space_mix", 0.45f }, { "space_size", 0.8f }, { "space_color", 1 }, { "space_predelay", 0.04f }, { "shimmer", 0.0f },
            { "master", 0.95f } } },

        { "Pad", "salem drag", {
            { "smp_level", 0.8f }, { "smp_mode", 2 }, { "smp_source", 1 }, { "smp_coarse", -12 }, { "smp_scan", 0.25f },
            { "smp_blur", 0.65f }, { "smp_whisper", 0.15f },
            { "osc1_wave", 3 }, { "osc1_coarse", -12 }, { "osc1_level", 0.2f }, { "osc2_level", 0.0f },
            { "harmony", 5 },
            { "attack", 0.8f }, { "decay", 1.0f }, { "sustain", 0.9f }, { "release", 3.0f },
            { "cutoff", 1600 }, { "resonance", 0.25f },
            { "drive", 0.25f }, { "crush_mix", 0.3f }, { "crush_bits", 8 }, { "crush_rate", 7000 },
            { "motion_depth", 0.25f }, { "motion_rate", 0 }, { "motion_shape", 0 },
            { "space_mix", 0.5f }, { "space_size", 0.9f }, { "space_color", 1 }, { "space_predelay", 0.08f }, { "shimmer", 0.1f },
            { "master", 0.25f } } },

        { "Pad", "botanica bloom", {
            { "smp_level", 0.7f }, { "smp_mode", 2 }, { "smp_source", 0 }, { "smp_scan", 0.3f },
            { "smp_blur", 0.5f }, { "smp_shift", 0.15f }, { "smp_sparse", 0.2f },
            { "osc1_wave", 0 }, { "osc1_level", 0.35f },
            { "osc2_wave", 1 }, { "osc2_coarse", 12 }, { "osc2_fine", 0 }, { "osc2_level", 0.2f },
            { "attack", 0.5f }, { "decay", 1.0f }, { "sustain", 0.8f }, { "release", 2.5f },
            { "cutoff", 6000 }, { "resonance", 0.2f },
            { "lfo_rate", 0.25f }, { "lfo_amount", 0.2f }, { "lfo_target", 1 },
            { "crystal_mix", 0.2f }, { "crystal_feedback", 0.5f },
            { "space_mix", 0.5f }, { "space_size", 0.75f }, { "shimmer", 0.6f }, { "master", 0.6f } } },

        //==============================================================================
        // PLUCK — стеклянный щипок, поломанная музыкальная шкатулка, гиперпоп-аккорды
        { "Pluck", "glass petal", {
            { "smp_level", 0.8f }, { "smp_mode", 0 }, { "smp_source", 0 }, { "smp_loop", 0 },
            { "osc1_wave", 1 }, { "osc1_level", 0.5f }, { "osc2_level", 0.0f },
            { "attack", 0.002f }, { "decay", 0.45f }, { "sustain", 0.0f }, { "release", 0.5f },
            { "cutoff", 7000 }, { "resonance", 0.2f }, { "filter_env", 0.2f },
            { "crystal_mix", 0.3f }, { "crystal_feedback", 0.5f },
            { "space_mix", 0.35f }, { "space_size", 0.6f }, { "shimmer", 0.5f }, { "master", 1.0f } } },

        { "Pluck", "aphex music box", {
            { "osc1_wave", 0 }, { "osc1_level", 1.0f },
            { "osc2_wave", 0 }, { "osc2_coarse", 19 }, { "osc2_fine", 0 }, { "osc2_level", 0.3f },
            { "osc3_wave", 3 }, { "osc3_coarse", 12 }, { "osc3_level", 0.08f },
            { "attack", 0.001f }, { "decay", 0.7f }, { "sustain", 0.0f }, { "release", 0.6f },
            { "crush_mix", 0.35f }, { "crush_bits", 10 }, { "crush_rate", 11000 },
            { "lfo_rate", 0.3f }, { "lfo_amount", 0.03f }, { "lfo_target", 0 },
            { "space_mix", 0.3f }, { "space_size", 0.5f }, { "space_color", 1 }, { "master", 1.0f } } },

        { "Pluck", "hyper pluck", {
            { "osc1_wave", 2 }, { "osc1_level", 1.0f },
            { "osc2_wave", 2 }, { "osc2_coarse", 0 }, { "osc2_fine", 15 }, { "osc2_level", 0.7f },
            { "osc3_wave", 3 }, { "osc3_coarse", 12 }, { "osc3_level", 0.2f },
            { "attack", 0.001f }, { "decay", 0.18f }, { "sustain", 0.0f }, { "release", 0.2f },
            { "cutoff", 900 }, { "resonance", 0.5f }, { "filter_env", 0.75f },
            { "harmony", 4 }, { "voices", 12 },
            { "drive", 0.2f }, { "drive_mode", 1 },
            { "crystal_mix", 0.15f }, { "crystal_pitch", 1 }, { "crystal_feedback", 0.35f },
            { "space_mix", 0.25f }, { "space_size", 0.5f }, { "shimmer", 0.3f }, { "master", 1.0f } } },

        //==============================================================================
        // ATMOSPHERE — дождь южного Лондона, ведьминский туман, лесное радио
        { "Atmosphere", "south london rain", {
            { "texture_type", 2 }, { "texture_amount", 0.9f }, { "texture_tone", 0.55f }, { "texture_density", 0.55f },
            { "texture_variation", 0.7f }, { "texture_seed", 23 }, { "texture_follow", 1 }, { "texture_width", 1.0f },
            { "smp_level", 0.5f }, { "smp_mode", 1 }, { "smp_source", 4 }, { "smp_position", 0.3f }, { "smp_scan", 0.15f },
            { "smp_grain", 0.2f }, { "smp_density", 18 }, { "smp_spray", 0.5f }, { "smp_jitter", 0.2f }, { "smp_reverse", 0.3f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f },
            { "attack", 1.5f }, { "sustain", 0.9f }, { "release", 3.0f }, { "cutoff", 3000 },
            { "crush_mix", 0.15f }, { "crush_bits", 12 },
            { "space_mix", 0.45f }, { "space_size", 0.85f }, { "space_color", 1 }, { "master", 0.7f } } },

        { "Atmosphere", "witch fog", {
            { "texture_type", 0 }, { "texture_amount", 0.8f }, { "texture_tone", 0.35f }, { "texture_density", 0.3f },
            { "texture_variation", 0.8f }, { "texture_seed", 66 }, { "texture_follow", 1 },
            { "smp_level", 0.6f }, { "smp_mode", 1 }, { "smp_source", 3 }, { "smp_coarse", -12 }, { "smp_scan", -0.3f },
            { "smp_grain", 0.3f }, { "smp_density", 12 }, { "smp_spray", 1.0f }, { "smp_reverse", 0.5f },
            { "osc1_wave", 4 }, { "osc1_level", 0.08f }, { "osc2_level", 0.0f },
            { "attack", 2.0f }, { "sustain", 0.9f }, { "release", 4.0f },
            { "cutoff", 1200 }, { "resonance", 0.3f }, { "drive", 0.2f },
            { "motion_depth", 0.2f }, { "motion_rate", 0 }, { "motion_shape", 0 },
            { "space_mix", 0.55f }, { "space_size", 0.95f }, { "space_color", 1 }, { "shimmer", 0.05f }, { "master", 0.85f } } },

        { "Atmosphere", "forest radio", {
            { "texture_type", 3 }, { "texture_amount", 0.5f }, { "texture_tone", 0.7f }, { "texture_density", 0.35f },
            { "texture_variation", 0.8f }, { "texture_seed", 404 }, { "texture_follow", 1 },
            { "smp_level", 0.5f }, { "smp_mode", 2 }, { "smp_source", 2 }, { "smp_sparse", 0.7f }, { "smp_blur", 0.4f },
            { "smp_scan", 0.2f }, { "smp_shift", 0.2f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f }, { "harmony", 2 },
            { "attack", 1.0f }, { "sustain", 0.9f }, { "release", 3.0f },
            { "crystal_mix", 0.3f }, { "crystal_pitch", 3 }, { "crystal_feedback", 0.5f },
            { "space_mix", 0.5f }, { "space_size", 0.7f }, { "shimmer", 0.5f }, { "master", 0.65f } } },

        //==============================================================================
        // SPECTRAL — расслоённый хор, замороженный голос-призрак, «цветущий» металл
        { "Spectral", "prism choir", {
            { "smp_level", 0.9f }, { "smp_mode", 2 }, { "smp_source", 1 }, { "smp_scan", 0.5f },
            { "smp_shift", 0.35f }, { "smp_sparse", 0.45f }, { "smp_blur", 0.3f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f },
            { "attack", 0.3f }, { "sustain", 0.9f }, { "release", 1.5f },
            { "space_mix", 0.35f }, { "space_size", 0.6f }, { "shimmer", 0.4f }, { "master", 1.0f } } },

        { "Spectral", "ghost freeze", {
            { "smp_level", 1.0f }, { "smp_mode", 2 }, { "smp_source", 4 }, { "smp_scan", 0.0f }, { "smp_position", 0.42f },
            { "smp_blur", 0.8f }, { "smp_whisper", 0.55f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f },
            { "attack", 0.6f }, { "sustain", 0.9f }, { "release", 2.5f }, { "cutoff", 5000 },
            { "crush_mix", 0.1f }, { "crush_bits", 10 },
            { "space_mix", 0.45f }, { "space_size", 0.8f }, { "space_color", 1 }, { "master", 1.0f } } },

        { "Spectral", "metal bloom", {
            { "smp_level", 0.9f }, { "smp_mode", 2 }, { "smp_source", 2 }, { "smp_scan", 0.15f },
            { "smp_shift", -0.4f }, { "smp_sparse", 0.7f }, { "smp_blur", 0.2f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f }, { "harmony", 2 },
            { "attack", 0.02f }, { "decay", 2.0f }, { "sustain", 0.4f }, { "release", 2.0f },
            { "drive", 0.2f }, { "drive_mode", 1 },
            { "crystal_mix", 0.25f }, { "crystal_feedback", 0.55f },
            { "space_mix", 0.4f }, { "space_size", 0.7f }, { "shimmer", 0.5f }, { "master", 0.6f } } },

        //==============================================================================
        // GRANULAR — облако вокальных чопов, стеклянный дождь, рой дыхания
        { "Granular", "vox cloud", {
            { "smp_level", 0.9f }, { "smp_mode", 1 }, { "smp_source", 4 }, { "smp_position", 0.1f }, { "smp_scan", 0.3f },
            { "smp_grain", 0.12f }, { "smp_density", 40 }, { "smp_spray", 0.6f }, { "smp_reverse", 0.3f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f },
            { "attack", 0.4f }, { "sustain", 0.9f }, { "release", 2.0f }, { "cutoff", 6000 },
            { "texture_type", 1 }, { "texture_amount", 0.25f }, { "texture_seed", 12 },
            { "crush_mix", 0.15f }, { "crush_bits", 12 },
            { "space_mix", 0.4f }, { "space_size", 0.7f }, { "space_color", 1 }, { "master", 1.0f } } },

        { "Granular", "glass rain", {
            { "smp_level", 0.8f }, { "smp_mode", 1 }, { "smp_source", 0 }, { "smp_scan", 0.5f },
            { "smp_grain", 0.05f }, { "smp_density", 80 }, { "smp_spray", 0.5f }, { "smp_jitter", 0.45f }, { "smp_reverse", 0.2f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f },
            { "attack", 0.1f }, { "decay", 1.5f }, { "sustain", 0.6f }, { "release", 1.8f },
            { "crystal_mix", 0.25f }, { "crystal_feedback", 0.4f },
            { "space_mix", 0.4f }, { "space_size", 0.7f }, { "shimmer", 0.55f }, { "master", 1.0f } } },

        { "Granular", "breath swarm", {
            { "smp_level", 1.0f }, { "smp_mode", 1 }, { "smp_source", 3 }, { "smp_scan", -0.3f },
            { "smp_grain", 0.3f }, { "smp_density", 15 }, { "smp_spray", 1.0f }, { "smp_reverse", 0.5f },
            { "osc1_level", 0.0f }, { "osc2_level", 0.0f },
            { "attack", 0.7f }, { "sustain", 0.9f }, { "release", 2.5f },
            { "motion_depth", 0.4f }, { "motion_rate", 1 }, { "motion_shape", 0 },
            { "vowel", 0.7f }, { "vowel_mix", 0.2f }, { "drive", 0.15f },
            { "space_mix", 0.4f }, { "space_size", 0.8f }, { "space_color", 1 }, { "master", 1.0f } } },
    };

    return presets;
}
