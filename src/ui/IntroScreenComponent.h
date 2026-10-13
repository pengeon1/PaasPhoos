#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace paasphoos
{

class IntroScreenComponent : public juce::Component,
                             public juce::Timer
{
public:
    IntroScreenComponent();
    ~IntroScreenComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void show(bool animated = true);
    void dismiss();

    juce::String getMusicianName() const;
    juce::String getInstrumentRole() const;

    std::function<void(const juce::String& name, const juce::String& role)> onStudioEntered;

private:
    void loadSavedProfile();
    void saveProfile(const juce::String& name, const juce::String& role);
    void handleEnter();

    // Visual animation state
    float animationPhase{0.0f};
    float fadeAlpha{1.0f};
    bool isFadingOut{false};

    // Brand Assets
    juce::Image logoImage;

    // UI Controls
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label taglineLabel;

    juce::Component cardPanel;
    juce::Label cardHeaderLabel;
    juce::Label cardPromptLabel;
    juce::Label nameFieldLabel;
    juce::TextEditor nameEditor;
    juce::Label roleFieldLabel;
    juce::ComboBox roleCombo;
    juce::TextButton enterButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IntroScreenComponent)
};

} // namespace paasphoos
