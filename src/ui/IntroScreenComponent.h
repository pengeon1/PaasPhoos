#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>
#include "../common/ThemeConfig.h"
#include "../common/TweenEngine.h"

namespace paasphoos
{

/**
 * Redesigned, ultra-smooth PaasPhoos Startup Experience.
 * Streamlined: Starts directly with the PaasPhoos animation intro, zero jitter.
 */
class IntroScreenComponent : public juce::Component,
                             public juce::Timer
{
public:
    IntroScreenComponent();
    ~IntroScreenComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // User interactions
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDown(const juce::MouseEvent& event) override;

    // Startup flow controls
    void startExperience(bool isFirstRun = true, bool isPreviewMode = false);
    void skipToNameScreen();
    void replayIntro();

    juce::String getMusicianName() const;
    juce::String getInstrumentRole() const;
    int getSelectedRoleIndex() const { return selectedRoleIndex; }
    juce::Colour getSelectedRoleColor() const { return ThemeConfig::getRoleColor(selectedRoleIndex); }

    // Callback when transitioning into studio:
    // (name, role, roleColor, avatarOrbScreenPosition)
    std::function<void(const juce::String& name, const juce::String& role, juce::Colour roleColor, juce::Rectangle<float> orbTargetRect)> onStudioEntered;

    // Standalone preview mode configuration
    void setPreviewMode(bool preview) { isPreviewMode = preview; }
    bool isPreviewActive() const { return isPreviewMode; }

    // Programmatic frame capture for verification
    juce::Image captureFrameAtTime(float timestampSeconds, int width, int height);

private:
    void setupRoleChips();
    void loadSavedProfile();
    void saveProfile(const juce::String& name, const juce::String& role);
    void validateAndSubmit();
    void triggerShakeAnimation();
    void playSynthesizedKickWhoosh();
    void updateCachedAuroraMesh(int width, int height);

    // Procedural gradient avatar orb generator
    void drawAvatarOrb(juce::Graphics& g, const juce::Rectangle<float>& bounds, const juce::String& name, float alpha = 1.0f);

    // Painting sub-phases (Streamlined PaasPhoos animation)
    void paintAuroraMesh(juce::Graphics& g, const juce::Rectangle<float>& bounds, float t);
    void paintPaasPhoosIntroAnimation(juce::Graphics& g, const juce::Rectangle<float>& bounds, float t);
    void paintWordmarkLetters(juce::Graphics& g, float centerX, float centerY, float t);
    void paintGlassCard(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    void paintAvatarFlightTransition(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    // State
    enum class State
    {
        IntroAnimation,
        NameInputScreen,
        TransitioningToApp,
        Completed
    };

    State currentState{State::IntroAnimation};
    float elapsedTime{0.0f};
    float totalIntroDuration{ThemeConfig::kIntroTotalDurationFirstRun};
    bool isFirstRun{true};
    bool isPreviewMode{false};
    bool isLiteMode{false};
    bool isReducedMotion{false};

    // Card shake animation
    float shakePhase{0.0f};
    float shakeIntensity{0.0f};

    // Transition animation
    float transitionProgress{0.0f};
    juce::Rectangle<float> flyingOrbCurrentBounds;
    juce::Rectangle<float> flyingOrbTargetBounds;

    // Fast off-screen pre-rendered background cache (zero CPU stutter)
    juce::Image cachedAuroraMesh;

    // UI Controls (Name Input Screen)
    juce::Label cardTitleLabel;
    juce::Label cardSubtitleLabel;
    juce::TextEditor nameEditor;
    float nameEditorFocusGlow{0.0f};

    // Role Selection Chips
    struct RoleChipButton : public juce::Button
    {
        RoleChipButton(const juce::String& name, int roleIdx, juce::Colour col);
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        int roleIndex;
        juce::Colour roleColor;
    };

    std::vector<std::unique_ptr<RoleChipButton>> roleButtons;
    int selectedRoleIndex{0}; // 0 = Vocals, 1 = Keys, 2 = Guitar, 3 = Drums, 4 = Other

    // Action Buttons
    struct GradientEnterButton : public juce::Button
    {
        GradientEnterButton();
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    };

    GradientEnterButton enterButton;
    juce::TextButton skipButton;
    juce::TextButton replayButton;

    // Logo image if present
    juce::Image logoImage;

    // Performance tracking
    double lastFrameTimeSeconds{0.0};
    int consecutiveSlowFrames{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IntroScreenComponent)
};

} // namespace paasphoos
