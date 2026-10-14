#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_core/juce_core.h>

namespace paasphoos
{

/**
 * Global Design System Tokens & Storyboard Configuration for PaasPhoos.
 * Editable config for colors, timings, tagline, lite mode, and audio whoosh toggle.
 */
struct ThemeConfig
{
    // ==========================================
    // Visual Palette Tokens
    // ==========================================
    static constexpr uint32_t kBaseBackgroundHex    = 0xff0b0d12; // Pure dark base #0B0D12
    static constexpr uint32_t kSurfaceHex           = 0xff12151c; // Surface #12151C
    static constexpr uint32_t kSurfaceElevatedHex   = 0xff1a1e28; // Elevated card #1A1E28
    static constexpr uint32_t kBorderSubtleHex      = 0xff262b3a; // Subtle border stroke
    static constexpr uint32_t kBorderHighlightHex   = 0xff3b435b; // 1px highlight stroke

    static constexpr uint32_t kTextPrimaryHex       = 0xfff2f4f8; // High contrast text #F2F4F8
    static constexpr uint32_t kTextMutedHex         = 0xff8a92a6; // Secondary/muted text #8A92A6
    static constexpr uint32_t kTextDimHex           = 0xff5a6275; // Dim/placeholder text

    // Accent Gradient
    static constexpr uint32_t kAccentVioletHex      = 0xff7c5cff; // Violet #7C5CFF
    static constexpr uint32_t kAccentBlueHex        = 0xff3da5ff; // Electric Blue #3DA5FF
    static constexpr uint32_t kAccentCyanHex        = 0xff38e1c6; // Cyan #38E1C6
    static constexpr uint32_t kAccentMagentaHex     = 0xffc084fc; // Soft Violet #C084FC
    static constexpr uint32_t kAccentAmberHex       = 0xffa78bfa; // Soft Violet #A78BFA

    // Role Colors (Strictly Blue - Violet - Cyan Palette)
    static constexpr uint32_t kRoleVocalsHex        = 0xff8b5cf6; // Electric Violet #8B5CF6
    static constexpr uint32_t kRoleKeysHex          = 0xff60a5fa; // Sky Blue #60A5FA
    static constexpr uint32_t kRoleGuitar1Hex       = 0xff3da5ff; // Electric Blue #3DA5FF
    static constexpr uint32_t kRoleGuitar2Hex       = 0xff6366f1; // Deep Indigo #6366F1
    static constexpr uint32_t kRoleGuitar3Hex       = 0xffa78bfa; // Lavender Violet #A78BFA
    static constexpr uint32_t kRoleDrumsHex         = 0xff818cf8; // Indigo #818CF8
    static constexpr uint32_t kRoleOtherHex         = 0xff38e1c6; // Cyan #38E1C6


    // Juce Colour accessors
    static juce::Colour getBaseBackground()    { return juce::Colour(kBaseBackgroundHex); }
    static juce::Colour getSurface()           { return juce::Colour(kSurfaceHex); }
    static juce::Colour getSurfaceElevated()   { return juce::Colour(kSurfaceElevatedHex); }
    static juce::Colour getBorderSubtle()      { return juce::Colour(kBorderSubtleHex); }
    static juce::Colour getBorderHighlight()   { return juce::Colour(kBorderHighlightHex); }
    static juce::Colour getTextPrimary()       { return juce::Colour(kTextPrimaryHex); }
    static juce::Colour getTextMuted()         { return juce::Colour(kTextMutedHex); }
    static juce::Colour getTextDim()           { return juce::Colour(kTextDimHex); }
    static juce::Colour getAccentViolet()      { return juce::Colour(kAccentVioletHex); }
    static juce::Colour getAccentBlue()        { return juce::Colour(kAccentBlueHex); }
    static juce::Colour getAccentCyan()        { return juce::Colour(kAccentCyanHex); }

    static juce::Colour getRoleColor(int roleIndex)
    {
        switch (roleIndex)
        {
            case 0: return juce::Colour(kRoleVocalsHex);
            case 1: return juce::Colour(kRoleKeysHex);
            case 2: return juce::Colour(kRoleGuitar1Hex);
            case 3: return juce::Colour(kRoleGuitar2Hex);
            case 4: return juce::Colour(kRoleGuitar3Hex);
            case 5: return juce::Colour(kRoleDrumsHex);
            default: return juce::Colour(kRoleOtherHex);
        }
    }

    static juce::Colour getRoleColorByName(const juce::String& role)
    {
        auto r = role.toLowerCase();
        if (r.contains("vocal"))   return juce::Colour(kRoleVocalsHex);
        if (r.contains("key") || r.contains("piano") || r.contains("synth")) return juce::Colour(kRoleKeysHex);
        if (r.contains("lead"))    return juce::Colour(kRoleGuitar1Hex);
        if (r.contains("rhythm"))  return juce::Colour(kRoleGuitar2Hex);
        if (r.contains("bass"))    return juce::Colour(kRoleGuitar3Hex);
        if (r.contains("guitar"))  return juce::Colour(kRoleGuitar1Hex);
        if (r.contains("drum") || r.contains("percussion")) return juce::Colour(kRoleDrumsHex);
        return juce::Colour(kRoleOtherHex);
    }

    // ==========================================
    // Storyboard Timings (in seconds)
    // Streamlined: Starts directly from PaasPhoos waveform & logo animation
    // ==========================================
    static constexpr float kIntroTotalDurationFirstRun   = 2.6f;  // First run ~2.6 s
    static constexpr float kIntroTotalDurationSubsequent = 1.0f;  // Later launches ~1.0 s

    static constexpr float kPhaseWaveformPulseEnd = 0.8f;  // 0.0 - 0.8s: Audio waveform emergence & kick pulse
    static constexpr float kPhaseWordmarkEnd      = 1.5f;  // 0.8 - 1.5s: PAASPHOOS wordmark reveal & glowing letters
    static constexpr float kPhaseTaglineEnd       = 2.0f;  // 1.5 - 2.0s: Tagline reveal ("Jam together. Even when you're apart.")
    static constexpr float kPhaseTransitionStart  = 2.0f;  // 2.0 - 2.6s: Logo glides up to header, card glides in

    // Tagline text
    static constexpr const char* kTaglineText = "Jam together. Even when you're apart.";

    // ==========================================
    // Performance & Lite Mode Settings
    // ==========================================
    static constexpr float kFrameTimeThresholdMsForLiteMode = 20.0f; // >20ms frame time triggers Lite mode
    static inline bool gForceLiteMode = false;
    static inline bool gDisableReducedMotionCheck = false;

    // ==========================================
    // Optional In-Code Synthesized Audio
    // ==========================================
    // Quiet whoosh + kick at beat moment (<= -24 dBFS, muteable, off by default)
    static inline bool gEnableIntroAudioSynth = false;

    // ==========================================
    // Profile Storage Helper
    // ==========================================
    static bool hasSavedProfile()
    {
        auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PaasPhoos");
        auto propFile = appDir.getChildFile("profile.properties");
        if (!propFile.existsAsFile())
            return false;

        juce::PropertiesFile::Options opts;
        opts.applicationName = "PaasPhoos";
        opts.filenameSuffix = "properties";
        opts.storageFormat = juce::PropertiesFile::storeAsXML;
        juce::PropertiesFile props(propFile, opts);
        return props.getValue("musicianName", "").trim().isNotEmpty();
    }
};

} // namespace paasphoos
