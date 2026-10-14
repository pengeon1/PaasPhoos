#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../audio/AudioEngine.h"
#include "../common/ThemeConfig.h"
#include "../common/TweenEngine.h"
#include "../common/Typography.h"
#include "AudioSettingsPanel.h"
#include "SessionPanel.h"
#include "MixerPanel.h"
#include "TunerComponent.h"
#include "MetronomeComponent.h"
#include "EffectsPanel.h"
#include "DrumPanel.h"
#include "RehearsalToolsPanel.h"
#include "IntroScreenComponent.h"

namespace paasphoos
{

class MainComponent : public juce::Component,
                      public juce::Timer
{
public:
    explicit MainComponent(AudioEngine& engine, bool introPreviewMode = false);
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void triggerPanelEntranceAnimation();
    void switchView(int viewIndex);
    void exportScreenshots(const juce::File& outputDir);

private:
    void updateProfileBadge();

    AudioEngine& audioEngine;
    bool isIntroPreviewMode{false};

    // Header Components
    juce::Image logoImage;
    PaasPhoosWordmarkComponent logoWordmark{22.0f};
    juce::Label subtitleLabel;
    juce::Label modeBadgeLabel;
    juce::Label cpuBadgeLabel;

    // Musician Profile Button (Circular Avatar Orb + Musician Username)
    class CircularProfileButton : public juce::Button
    {
    public:
        CircularProfileButton();
        void setProfile(const juce::String& name, const juce::String& role, juce::Colour color);
        juce::Rectangle<float> getAvatarOrbBoundsInParent() const;
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    private:
        void drawAvatarCircle(juce::Graphics& g, const juce::Rectangle<float>& orbBounds,
                              const juce::String& name, juce::Colour col, bool highlighted, bool isDown);

        juce::String musicianName{"Musician"};
        juce::String instrumentRole{"Vocals"};
        juce::Colour roleColor{juce::Colour(ThemeConfig::kAccentVioletHex)};
    };

    CircularProfileButton profileButton;
    juce::Colour currentMusicianRoleColor;

    // Navigation Buttons
    juce::TextButton viewMixerBtn;
    juce::TextButton viewFxBtn;
    juce::TextButton viewDrumsBtn;
    juce::TextButton viewRehearsalBtn;
    int currentViewIndex{0};

    // Sub-panels
    SessionPanel sessionPanel;
    AudioSettingsPanel audioSettingsPanel;
    MetronomeComponent metronomeComponent;
    MixerPanel mixerPanel;
    TunerComponent tunerComponent;
    EffectsPanel effectsPanel;
    DrumPanel drumPanel;
    RehearsalToolsPanel rehearsalToolsPanel;

    // Status Bar
    juce::Label statusBarLabel;
    juce::Label mmcssStatusLabel;

    // Staggered Entrance Animation State
    bool isEntranceAnimating{false};
    float entranceTimeSeconds{0.0f};

    // Intro Animation & Profile Setup Overlay
    IntroScreenComponent introScreen;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace paasphoos
