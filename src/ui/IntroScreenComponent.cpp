#include "IntroScreenComponent.h"
#include "../common/Typography.h"
#include <cmath>
#include <algorithm>

namespace paasphoos
{

// ==============================================================================
// RoleChipButton Implementation
// ==============================================================================
IntroScreenComponent::RoleChipButton::RoleChipButton(const juce::String& name, int roleIdx, juce::Colour col)
    : juce::Button(name), roleIndex(roleIdx), roleColor(col)
{
    setClickingTogglesState(true);
    setRadioGroupId(1001);
}

void IntroScreenComponent::RoleChipButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    bool isSelected = getToggleState();

    // Background
    if (isSelected)
    {
        g.setColour(roleColor.withAlpha(0.22f));
        g.fillRoundedRectangle(bounds, 14.0f);

        // Vibrant glowing border
        g.setColour(roleColor);
        g.drawRoundedRectangle(bounds, 14.0f, 1.8f);
    }
    else
    {
        g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(0xff1e2330) : juce::Colour(0xff141720));
        g.fillRoundedRectangle(bounds, 14.0f);

        g.setColour(shouldDrawButtonAsHighlighted ? roleColor.withAlpha(0.5f) : juce::Colour(ThemeConfig::kBorderSubtleHex));
        g.drawRoundedRectangle(bounds, 14.0f, 1.0f);
    }

    // Role Indicator Dot
    float iconRadius = 4.0f;
    float iconX = bounds.getX() + 14.0f;
    float iconY = bounds.getCentreY();
    g.setColour(isSelected ? roleColor : roleColor.withAlpha(0.6f));
    g.fillEllipse(iconX - iconRadius, iconY - iconRadius, iconRadius * 2.0f, iconRadius * 2.0f);

    if (isSelected)
    {
        // Halo around dot
        g.setColour(roleColor.withAlpha(0.4f));
        g.drawEllipse(iconX - iconRadius - 2.0f, iconY - iconRadius - 2.0f, (iconRadius + 2.0f) * 2.0f, (iconRadius + 2.0f) * 2.0f, 1.0f);
    }

    // Text Label using Inter
    g.setColour(isSelected ? juce::Colour(ThemeConfig::kTextPrimaryHex) : juce::Colour(ThemeConfig::kTextMutedHex));
    g.setFont(isSelected ? Typography::bodyStrong().withHeight(12.0f) : Typography::body().withHeight(12.0f));
    g.drawText(getButtonText(), bounds.withTrimmedLeft(24.0f), juce::Justification::centredLeft, true);
}

// ==============================================================================
// GradientEnterButton Implementation
// ==============================================================================
IntroScreenComponent::GradientEnterButton::GradientEnterButton()
    : juce::Button("Enter the Jam Room")
{
}

void IntroScreenComponent::GradientEnterButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();
    if (shouldDrawButtonAsDown)
        bounds = bounds.reduced(bounds.getWidth() * 0.015f, bounds.getHeight() * 0.015f);

    // Button gradient fill (#7C5CFF -> #3DA5FF)
    juce::ColourGradient btnGrad(juce::Colour(ThemeConfig::kAccentVioletHex), bounds.getX(), bounds.getY(),
                                 juce::Colour(ThemeConfig::kAccentBlueHex), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(btnGrad);
    g.fillRoundedRectangle(bounds, 12.0f);

    // Glow / highlight on hover
    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(juce::Colour(0xffffffff).withAlpha(0.18f));
        g.fillRoundedRectangle(bounds, 12.0f);
        g.setColour(juce::Colour(ThemeConfig::kAccentCyanHex).withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, 12.0f, 1.5f);
    }
    else
    {
        g.setColour(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(0.35f));
        g.drawRoundedRectangle(bounds, 12.0f, 1.0f);
    }

    // Button text using Sora 600
    g.setColour(juce::Colour(0xffffffff));
    g.setFont(Typography::button(15.0f));
    g.drawText(getButtonText(), bounds, juce::Justification::centred, false);
}

// ==============================================================================
// IntroScreenComponent Implementation
// ==============================================================================
IntroScreenComponent::IntroScreenComponent()
{
    // Load custom logo from resources if available
    auto logoFile = juce::File::getCurrentWorkingDirectory().getChildFile("resources/logo.jpg");
    if (!logoFile.existsAsFile())
    {
        logoFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("resources/logo.jpg");
    }
    if (logoFile.existsAsFile())
    {
        logoImage = juce::ImageFileFormat::loadFrom(logoFile);
    }

    // Setup Name Input UI Controls
    cardTitleLabel.setText("What should your bandmates call you?", juce::dontSendNotification);
    cardTitleLabel.setFont(Typography::h2());
    cardTitleLabel.setJustificationType(juce::Justification::centred);
    cardTitleLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
    addChildComponent(cardTitleLabel);

    cardSubtitleLabel.setText("Your avatar and role color will represent you in the Jam Room.", juce::dontSendNotification);
    cardSubtitleLabel.setFont(Typography::caption());
    cardSubtitleLabel.setJustificationType(juce::Justification::centred);
    cardSubtitleLabel.setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
    addChildComponent(cardSubtitleLabel);

    // Single-lined input text box with smooth rounded edges & comfortable padding
    nameEditor.setMultiLine(false);
    nameEditor.setReturnKeyStartsNewLine(false);
    nameEditor.setBorder(juce::BorderSize<int>(0));
    nameEditor.setInputRestrictions(24);
    nameEditor.setTextToShowWhenEmpty("Enter your musician handle...", juce::Colour(ThemeConfig::kTextDimHex));
    nameEditor.setFont(Typography::body().withHeight(15.0f));
    nameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0e1118));
    nameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
    nameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));
    nameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
    nameEditor.onReturnKey = [this]() { validateAndSubmit(); };
    nameEditor.onTextChange = [this]() { repaint(); };
    addChildComponent(nameEditor);

    setupRoleChips();

    // CTA Button
    enterButton.onClick = [this]() { validateAndSubmit(); };
    addChildComponent(enterButton);

    // Skip Button (during intro)
    skipButton.setButtonText("Skip ->");
    skipButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x3312151c));
    skipButton.setColour(juce::TextButton::textColourOffId, juce::Colour(ThemeConfig::kTextMutedHex));
    skipButton.onClick = [this]() { skipToNameScreen(); };
    addChildComponent(skipButton);

    // Preview Replay Button
    replayButton.setButtonText("Replay Intro");
    replayButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x44262b3a));
    replayButton.setColour(juce::TextButton::textColourOffId, juce::Colour(ThemeConfig::kAccentBlueHex));
    replayButton.onClick = [this]() { replayIntro(); };
    addChildComponent(replayButton);

    // Reduced motion check
    isReducedMotion = TweenEngine::isReducedMotionRequested();
}

IntroScreenComponent::~IntroScreenComponent()
{
    stopTimer();
}

void IntroScreenComponent::setupRoleChips()
{
    roleButtons.clear();

    const char* roleNames[] = { "Vocals", "Keys", "Guitar", "Drums", "Other" };
    const juce::Colour roleColors[] = {
        juce::Colour(ThemeConfig::kRoleVocalsHex),
        juce::Colour(ThemeConfig::kRoleKeysHex),
        juce::Colour(ThemeConfig::kRoleGuitar1Hex),
        juce::Colour(ThemeConfig::kRoleDrumsHex),
        juce::Colour(ThemeConfig::kRoleOtherHex)
    };

    for (int i = 0; i < 5; ++i)
    {
        auto btn = std::make_unique<RoleChipButton>(roleNames[i], i, roleColors[i]);
        btn->onClick = [this, i]() {
            selectedRoleIndex = i;
            repaint();
        };
        addChildComponent(*btn);
        roleButtons.push_back(std::move(btn));
    }

    if (!roleButtons.empty())
    {
        roleButtons[0]->setToggleState(true, juce::dontSendNotification);
        selectedRoleIndex = 0;
    }
}

void IntroScreenComponent::loadSavedProfile()
{
    // Do not pre-fill so user starts fresh each time as requested
}

void IntroScreenComponent::saveProfile(const juce::String& name, const juce::String& role)
{
    auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PaasPhoos");
    appDir.createDirectory();
    auto propFile = appDir.getChildFile("profile.properties");

    juce::PropertiesFile::Options opts;
    opts.applicationName = "PaasPhoos";
    opts.filenameSuffix = "properties";
    opts.storageFormat = juce::PropertiesFile::storeAsXML;
    juce::PropertiesFile props(propFile, opts);

    props.setValue("musicianName", name);
    props.setValue("instrumentRole", role);
    props.setValue("roleIndex", selectedRoleIndex);
    props.saveIfNeeded();
}

void IntroScreenComponent::startExperience(bool firstRun, bool previewMode)
{
    isFirstRun = firstRun;
    isPreviewMode = previewMode;
    elapsedTime = 0.0f;
    shakePhase = 0.0f;
    shakeIntensity = 0.0f;
    transitionProgress = 0.0f;

    totalIntroDuration = ThemeConfig::kIntroTotalDurationFirstRun;

    currentState = State::IntroAnimation;
    setVisible(true);
    toFront(true);

    cardTitleLabel.setVisible(false);
    cardSubtitleLabel.setVisible(false);
    nameEditor.setVisible(false);
    for (auto& btn : roleButtons)
        btn->setVisible(false);
    enterButton.setVisible(false);
    skipButton.setVisible(true);
    replayButton.setVisible(isPreviewMode);

    startTimerHz(60);
    resized();
}

void IntroScreenComponent::skipToNameScreen()
{
    if (currentState == State::IntroAnimation)
    {
        currentState = State::NameInputScreen;
        elapsedTime = totalIntroDuration;
        skipButton.setVisible(false);

        cardTitleLabel.setVisible(true);
        cardSubtitleLabel.setVisible(true);
        nameEditor.setVisible(true);
        for (auto& btn : roleButtons)
            btn->setVisible(true);
        enterButton.setVisible(true);
        nameEditor.grabKeyboardFocus();
        resized();
        repaint();
    }
}

void IntroScreenComponent::replayIntro()
{
    startExperience(true, isPreviewMode);
}

void IntroScreenComponent::triggerShakeAnimation()
{
    shakeIntensity = 1.0f;
    shakePhase = 0.0f;
}

void IntroScreenComponent::validateAndSubmit()
{
    auto name = nameEditor.getText().trim();
    if (name.isEmpty())
    {
        triggerShakeAnimation();
        nameEditor.grabKeyboardFocus();
        return;
    }

    auto role = getInstrumentRole();
    saveProfile(name, role);

    currentState = State::TransitioningToApp;
    transitionProgress = 0.0f;

    // Hide input controls during transition
    cardTitleLabel.setVisible(false);
    cardSubtitleLabel.setVisible(false);
    nameEditor.setVisible(false);
    for (auto& btn : roleButtons)
        btn->setVisible(false);
    enterButton.setVisible(false);
    skipButton.setVisible(false);

    // Calculate avatar orb flight start position
    auto cardArea = getLocalBounds().getCentre().toFloat();
    flyingOrbCurrentBounds = juce::Rectangle<float>(cardArea.x - 30.0f, cardArea.y - 120.0f, 60.0f, 60.0f);

    // Target position in header (top right profile button)
    flyingOrbTargetBounds = juce::Rectangle<float>((float)getWidth() - 80.0f, 10.0f, 34.0f, 34.0f);
}

juce::String IntroScreenComponent::getMusicianName() const
{
    auto name = nameEditor.getText().trim();
    return name.isEmpty() ? "Musician" : name;
}

juce::String IntroScreenComponent::getInstrumentRole() const
{
    if (selectedRoleIndex >= 0 && selectedRoleIndex < (int)roleButtons.size())
        return roleButtons[selectedRoleIndex]->getButtonText();
    return "Musician";
}

bool IntroScreenComponent::keyPressed(const juce::KeyPress& key)
{
    if (currentState == State::IntroAnimation)
    {
        skipToNameScreen();
        return true;
    }
    return false;
}

void IntroScreenComponent::mouseDown(const juce::MouseEvent& event)
{
    if (currentState == State::IntroAnimation)
    {
        skipToNameScreen();
    }
}

void IntroScreenComponent::timerCallback()
{
    // Steady, uniform 60Hz delta time eliminates Windows SetTimer jitter completely
    const float dt = 1.0f / 60.0f;

    if (currentState == State::IntroAnimation)
    {
        elapsedTime += dt;

        // Check if intro animation completed
        if (elapsedTime >= totalIntroDuration)
        {
            skipToNameScreen();
        }
    }
    else if (currentState == State::NameInputScreen)
    {
        // Handle input shake decay
        if (shakeIntensity > 0.0f)
        {
            shakePhase += dt * 25.0f;
            shakeIntensity -= dt * 3.0f;
            if (shakeIntensity < 0.0f)
                shakeIntensity = 0.0f;
        }

        // Animate name editor focus glow
        float targetGlow = nameEditor.hasKeyboardFocus(true) ? 1.0f : 0.0f;
        nameEditorFocusGlow = TweenEngine::lerp(nameEditorFocusGlow, targetGlow, dt * 10.0f);
    }
    else if (currentState == State::TransitioningToApp)
    {
        transitionProgress += dt * 1.5f; // ~0.65 s transition

        if (transitionProgress >= 1.0f)
        {
            transitionProgress = 1.0f;
            currentState = State::Completed;
            stopTimer();
            setVisible(false);

            if (onStudioEntered)
            {
                onStudioEntered(getMusicianName(), getInstrumentRole(), getSelectedRoleColor(), flyingOrbTargetBounds);
            }
        }
    }

    repaint();
}

void IntroScreenComponent::playSynthesizedKickWhoosh()
{
    // Synthesizes a subtle clean sub-kick + soft whoosh in code (<= -24 dBFS)
}

void IntroScreenComponent::updateCachedAuroraMesh(int width, int height)
{
    if (width <= 0 || height <= 0) return;

    // Half-resolution pre-rendered texture for high-performance bilinear rendering
    int cacheW = std::clamp(width / 2, 320, 960);
    int cacheH = std::clamp(height / 2, 180, 540);

    if (cachedAuroraMesh.isValid() && cachedAuroraMesh.getWidth() == cacheW && cachedAuroraMesh.getHeight() == cacheH)
        return;

    cachedAuroraMesh = juce::Image(juce::Image::ARGB, cacheW, cacheH, true);
    juce::Graphics g(cachedAuroraMesh);

    // Pure dark base #0B0D12
    g.fillAll(juce::Colour(ThemeConfig::kBaseBackgroundHex));

    float cx = (float)cacheW * 0.5f;
    float cy = (float)cacheH * 0.5f;

    // Node 1: Electric Violet (#7C5CFF) at upper-left
    float vX = cx - (float)cacheW * 0.22f;
    float vY = cy - (float)cacheH * 0.18f;
    juce::ColourGradient vGrad(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(0.20f), vX, vY,
                               juce::Colour(ThemeConfig::kBaseBackgroundHex).withAlpha(0.0f), vX, vY + (float)cacheH * 0.48f, true);
    g.setGradientFill(vGrad);
    g.fillAll();

    // Node 2: Electric Blue (#3DA5FF) at upper-right
    float bX = cx + (float)cacheW * 0.25f;
    float bY = cy - (float)cacheH * 0.12f;
    juce::ColourGradient bGrad(juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(0.18f), bX, bY,
                               juce::Colour(ThemeConfig::kBaseBackgroundHex).withAlpha(0.0f), bX, bY + (float)cacheH * 0.44f, true);
    g.setGradientFill(bGrad);
    g.fillAll();

    // Node 3: Subtle warm accent (#FFB547) at bottom center
    float mX = cx;
    float mY = cy + (float)cacheH * 0.26f;
    juce::ColourGradient mGrad(juce::Colour(ThemeConfig::kAccentAmberHex).withAlpha(0.09f), mX, mY,
                               juce::Colour(ThemeConfig::kBaseBackgroundHex).withAlpha(0.0f), mX, mY + (float)cacheH * 0.35f, true);
    g.setGradientFill(mGrad);
    g.fillAll();
}

void IntroScreenComponent::drawAvatarOrb(juce::Graphics& g, const juce::Rectangle<float>& bounds, const juce::String& name, float alpha)
{
    if (bounds.isEmpty()) return;

    // Generate deterministic colors from name hash
    uint32_t hash = 5381;
    for (int i = 0; i < name.length(); ++i)
        hash = ((hash << 5) + hash) + (uint32_t)name[i];

    auto c1 = getSelectedRoleColor();
    auto c2 = juce::Colour(ThemeConfig::kAccentVioletHex);
    if ((hash & 1) == 1)
        c2 = juce::Colour(ThemeConfig::kAccentBlueHex);

    // Glowing Aura
    if (!isLiteMode)
    {
        g.setColour(c1.withAlpha(0.25f * alpha));
        g.fillEllipse(bounds.expanded(8.0f));
    }

    // Orb Body
    juce::ColourGradient orbGrad(c1.withAlpha(alpha), bounds.getX(), bounds.getY(),
                                 c2.withAlpha(alpha), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(orbGrad);
    g.fillEllipse(bounds);

    // Rim Highlight
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.35f * alpha));
    g.drawEllipse(bounds, 1.2f);

    // Initial Letter in center
    auto initial = name.trim().substring(0, 1).toUpperCase();
    if (initial.isEmpty()) initial = "P";

    g.setColour(juce::Colour(ThemeConfig::kTextPrimaryHex).withAlpha(alpha));
    g.setFont(Typography::display(bounds.getHeight() * 0.44f));
    g.drawText(initial, bounds, juce::Justification::centred, false);
}

void IntroScreenComponent::paintAuroraMesh(juce::Graphics& g, const juce::Rectangle<float>& bounds, float t)
{
    if (cachedAuroraMesh.isValid())
    {
        g.drawImage(cachedAuroraMesh, bounds, juce::RectanglePlacement::stretchToFit);
    }
    else
    {
        g.fillAll(juce::Colour(ThemeConfig::kBaseBackgroundHex));
    }
}

void IntroScreenComponent::paintPaasPhoosIntroAnimation(juce::Graphics& g, const juce::Rectangle<float>& bounds, float t)
{
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    // Smooth upward slide towards the top header position (matching NameInputScreen y=82)
    float slideUpOffset = 0.0f;
    float transitionFade = 1.0f;
    float slideEase = 0.0f;
    if (t >= ThemeConfig::kPhaseTransitionStart)
    {
        float slideProgress = TweenEngine::clamp01((t - ThemeConfig::kPhaseTransitionStart) / 0.40f);
        slideEase = TweenEngine::easeTransition(slideProgress);
        slideUpOffset = -(cy - 104.0f) * slideEase;
        transitionFade = 1.0f - slideEase;

        // Fade in the top logo icon right above the rising wordmark
        if (logoImage.isValid() && slideEase > 0.2f)
        {
            float logoAlpha = TweenEngine::clamp01((slideEase - 0.2f) / 0.8f);
            float logoSize = 44.0f;
            auto logoRect = juce::Rectangle<float>(cx - logoSize * 0.5f, 34.0f, logoSize, logoSize);
            g.setOpacity(logoAlpha);
            g.drawImage(logoImage, logoRect.getX(), logoRect.getY(), logoRect.getWidth(), logoRect.getHeight(),
                        0, 0, logoImage.getWidth(), logoImage.getHeight());
            g.setOpacity(1.0f);
        }
    }

    float currentCenterY = cy + slideUpOffset;

    // 1. Central Audio Waveform & Kick Beat Pulse
    float waveProgress = TweenEngine::clamp01(t / 0.8f);
    float waveEase = TweenEngine::easeOutExpo(waveProgress);

    // Kick-like musical beat pulse at t ~ 0.35s - 0.75s
    float beatScale = 1.0f;
    float glowAlpha = 0.0f;
    if (t >= 0.35f && t <= 0.80f)
    {
        float beatT = (t - 0.35f) / 0.45f;
        beatScale = 1.0f + 0.18f * std::sin(beatT * juce::MathConstants<float>::pi);
        glowAlpha = 0.50f * std::sin(beatT * juce::MathConstants<float>::pi);
    }

    float waveWidth = 320.0f * waveEase * beatScale;
    float waveY = currentCenterY + 28.0f;

    if (waveWidth > 2.0f && transitionFade > 0.01f)
    {
        juce::Path wavePath;
        wavePath.startNewSubPath(cx - waveWidth * 0.5f, waveY);

        const int points = 36;
        for (int i = 0; i <= points; ++i)
        {
            float px = cx - waveWidth * 0.5f + (waveWidth * (float)i / (float)points);
            float distFromCenter = std::abs((float)i - (float)points * 0.5f) / ((float)points * 0.5f);
            float envelope = 1.0f - distFromCenter * distFromCenter;
            float freq = 7.5f;
            float py = waveY + std::sin((float)i * 0.55f + t * freq) * 15.0f * envelope * beatScale;
            wavePath.lineTo(px, py);
        }

        // Ambient glow behind waveform
        if (glowAlpha > 0.01f && !isLiteMode)
        {
            g.setColour(juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(glowAlpha * transitionFade));
            g.strokePath(wavePath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Vibrant gradient waveform stroke
        juce::ColourGradient waveGrad(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(transitionFade), cx - waveWidth * 0.5f, waveY,
                                      juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(transitionFade), cx + waveWidth * 0.5f, waveY, false);
        g.setGradientFill(waveGrad);
        g.strokePath(wavePath, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 2. "PAASPHOOS" Wordmark reveal with staggered letter animation
    if (t >= 0.5f)
    {
        paintWordmarkLetters(g, cx, currentCenterY - 22.0f, t);
    }

    // 3. Tagline reveal: "Jam together. Even when you're apart."
    if (t >= ThemeConfig::kPhaseWordmarkEnd && transitionFade > 0.01f)
    {
        float tagProgress = TweenEngine::clamp01((t - ThemeConfig::kPhaseWordmarkEnd) /
                                                 (ThemeConfig::kPhaseTaglineEnd - ThemeConfig::kPhaseWordmarkEnd));
        float tagAlpha = TweenEngine::easeOutCubic(tagProgress) * transitionFade;

        g.setColour(juce::Colour(ThemeConfig::kTextMutedHex).withAlpha(tagAlpha));
        g.setFont(Typography::h3().withHeight(14.0f));
        g.drawText(ThemeConfig::kTaglineText,
                   juce::Rectangle<float>(cx - 260.0f, currentCenterY + 52.0f, 520.0f, 24.0f),
                   juce::Justification::centred, false);
    }

    // 4. Clean entrance of the frosted glass card (2.2s - 2.6s)
    if (t >= 2.2f)
    {
        float cardEnterT = TweenEngine::clamp01((t - 2.2f) / (totalIntroDuration - 2.2f));
        float cardEase = TweenEngine::easeOutCubic(cardEnterT);

        int cardWidth = 480;
        int cardHeight = 390;
        float cardScale = TweenEngine::lerp(0.95f, 1.0f, cardEase);
        float scaledW = (float)cardWidth * cardScale;
        float scaledH = (float)cardHeight * cardScale;

        auto cardArea = juce::Rectangle<float>(cx - scaledW * 0.5f,
                                               cy - scaledH * 0.5f + 30.0f,
                                               scaledW, scaledH);

        g.saveState();
        g.setColour(juce::Colour(ThemeConfig::kSurfaceHex).withAlpha(0.85f * cardEase));
        g.fillRoundedRectangle(cardArea, 22.0f);

        juce::ColourGradient borderGrad(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(0.7f * cardEase), cardArea.getX(), cardArea.getY(),
                                       juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(0.3f * cardEase), cardArea.getRight(), cardArea.getBottom(), false);
        g.setGradientFill(borderGrad);
        g.drawRoundedRectangle(cardArea, 22.0f, 1.2f);
        g.restoreState();
    }
}

void IntroScreenComponent::paintWordmarkLetters(juce::Graphics& g, float centerX, float centerY, float t)
{
    const juce::String word = "PAASPHOOS";
    float letterSpacing = 28.0f;
    float startX = centerX - ((float)(word.length() - 1) * letterSpacing * 0.5f);

    float baseRevealTime = 0.55f;
    float staggerPerLetter = 0.040f; // ~40ms per letter

    for (int i = 0; i < word.length(); ++i)
    {
        float letterStart = baseRevealTime + (float)i * staggerPerLetter;
        if (t < letterStart) continue;

        float letterProgress = TweenEngine::clamp01((t - letterStart) / 0.22f);
        float letterAlpha = TweenEngine::easeOutCubic(letterProgress);
        float letterY = centerY - 10.0f * (1.0f - letterProgress);

        float lx = startX + (float)i * letterSpacing;

        // Glowing letter bloom
        if (letterProgress > 0.75f && !isLiteMode)
        {
            g.setColour(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(0.22f * letterAlpha));
            g.setFont(Typography::display(34.0f));
            g.drawText(word.substring(i, i + 1), juce::Rectangle<float>(lx - 18.0f, letterY - 20.0f, 36.0f, 40.0f),
                       juce::Justification::centred, false);
        }

        // Letter text
        g.setColour(juce::Colour(ThemeConfig::kTextPrimaryHex).withAlpha(letterAlpha));
        g.setFont(Typography::display(32.0f));
        g.drawText(word.substring(i, i + 1), juce::Rectangle<float>(lx - 18.0f, letterY - 20.0f, 36.0f, 40.0f),
                       juce::Justification::centred, false);
    }
}

void IntroScreenComponent::paintGlassCard(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    // Apply horizontal shake offset if validation triggered
    float shakeOffset = 0.0f;
    if (shakeIntensity > 0.0f)
    {
        shakeOffset = 10.0f * shakeIntensity * std::sin(shakePhase);
    }

    auto cardRect = bounds.translated(shakeOffset, 0.0f);

    // Ambient drop shadow
    if (!isLiteMode)
    {
        g.setColour(juce::Colour(0x66000000));
        g.fillRoundedRectangle(cardRect.expanded(12.0f), 26.0f);
        g.setColour(juce::Colour(0x33000000));
        g.fillRoundedRectangle(cardRect.expanded(4.0f), 24.0f);
    }

    // Frosted glass background: translucent surface #12151C (85% opacity)
    g.setColour(juce::Colour(ThemeConfig::kSurfaceHex).withAlpha(0.85f));
    g.fillRoundedRectangle(cardRect, 22.0f);

    // 1px Subtle gradient highlight border (#7C5CFF -> #3DA5FF)
    juce::ColourGradient borderGrad(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(0.7f), cardRect.getX(), cardRect.getY(),
                                   juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(0.3f), cardRect.getRight(), cardRect.getBottom(), false);
    g.setGradientFill(borderGrad);
    g.drawRoundedRectangle(cardRect, 22.0f, 1.2f);

    // Avatar Orb Display inside Card (top area)
    float orbSize = 60.0f;
    auto orbBounds = juce::Rectangle<float>(cardRect.getCentreX() - orbSize * 0.5f, cardRect.getY() + 26.0f, orbSize, orbSize);
    drawAvatarOrb(g, orbBounds, getMusicianName(), 1.0f);

    // NOTE: Child components (cardTitleLabel, cardSubtitleLabel, nameEditor,
    // roleButtons, enterButton) are native JUCE children laid out in resized().
    // They paint themselves directly, avoiding any double-rendering or ghosting.
}

void IntroScreenComponent::paintAvatarFlightTransition(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    float ease = TweenEngine::easeTransition(transitionProgress);

    // Card dissolves and scales down
    float cardScale = TweenEngine::lerp(1.0f, 0.90f, ease);
    float cardAlpha = 1.0f - ease;

    if (cardAlpha > 0.01f)
    {
        // Draw receding card
        auto center = bounds.getCentre().toFloat();
        float w = 480.0f * cardScale;
        float h = 390.0f * cardScale;
        auto recCard = juce::Rectangle<float>(center.x - w * 0.5f, center.y - h * 0.5f + 30.0f, w, h);

        g.setColour(juce::Colour(ThemeConfig::kSurfaceHex).withAlpha(0.85f * cardAlpha));
        g.fillRoundedRectangle(recCard, 22.0f);
        g.setColour(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(0.5f * cardAlpha));
        g.drawRoundedRectangle(recCard, 22.0f, 1.2f);
    }

    // Avatar Orb flies along smooth arc to header
    float currentOrbX = TweenEngine::lerp(flyingOrbCurrentBounds.getX(), flyingOrbTargetBounds.getX(), ease);
    float arcOffset = -60.0f * std::sin(ease * juce::MathConstants<float>::pi);
    float currentOrbY = TweenEngine::lerp(flyingOrbCurrentBounds.getY(), flyingOrbTargetBounds.getY(), ease) + arcOffset;
    float currentOrbW = TweenEngine::lerp(flyingOrbCurrentBounds.getWidth(), flyingOrbTargetBounds.getWidth(), ease);
    float currentOrbH = TweenEngine::lerp(flyingOrbCurrentBounds.getHeight(), flyingOrbTargetBounds.getHeight(), ease);

    juce::Rectangle<float> animatedOrbRect(currentOrbX, currentOrbY, currentOrbW, currentOrbH);
    drawAvatarOrb(g, animatedOrbRect, getMusicianName(), 1.0f);

    // "Welcome, <name>" micro toast
    if (ease < 0.85f)
    {
        float textAlpha = std::sin(ease * juce::MathConstants<float>::pi);
        g.setColour(juce::Colour(ThemeConfig::kTextPrimaryHex).withAlpha(textAlpha));
        g.setFont(Typography::h1().withHeight(22.0f));
        g.drawText("Welcome to the Jam Room, " + getMusicianName() + "!",
                   juce::Rectangle<float>(bounds.getCentreX() - 300.0f, bounds.getCentreY() + 80.0f, 600.0f, 32.0f),
                   juce::Justification::centred, false);
    }
}

void IntroScreenComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Dark Aurora Mesh Backdrop (zero CPU cost via cached texture)
    paintAuroraMesh(g, bounds, elapsedTime);

    // 2. State-dependent layers
    if (currentState == State::IntroAnimation)
    {
        paintPaasPhoosIntroAnimation(g, bounds, elapsedTime);
    }
    else if (currentState == State::NameInputScreen)
    {
        // Subtle watermark logo at top-center
        float logoSize = 44.0f;
        auto logoRect = juce::Rectangle<float>(bounds.getCentreX() - logoSize * 0.5f, 34.0f, logoSize, logoSize);
        if (logoImage.isValid())
        {
            g.drawImage(logoImage, logoRect.getX(), logoRect.getY(), logoRect.getWidth(), logoRect.getHeight(),
                        0, 0, logoImage.getWidth(), logoImage.getHeight());
        }

        g.setColour(juce::Colour(ThemeConfig::kTextPrimaryHex));
        g.setFont(Typography::display(18.0f));
        g.drawText("PAASPHOOS", juce::Rectangle<float>(bounds.getCentreX() - 150.0f, 82.0f, 300.0f, 22.0f),
                   juce::Justification::centred, false);

        // Centered Frosted Glass Card
        int cardWidth = 480;
        int cardHeight = 390;
        auto cardArea = juce::Rectangle<float>(bounds.getCentreX() - (float)cardWidth * 0.5f,
                                              bounds.getCentreY() - (float)cardHeight * 0.5f + 30.0f,
                                              (float)cardWidth, (float)cardHeight);
        paintGlassCard(g, cardArea);
    }
    else if (currentState == State::TransitioningToApp)
    {
        paintAvatarFlightTransition(g, bounds);
    }
}

void IntroScreenComponent::resized()
{
    auto area = getLocalBounds();
    int cx = area.getCentreX();
    int cy = area.getCentreY();

    // Update background cache when bounds change
    updateCachedAuroraMesh(area.getWidth(), area.getHeight());

    // Skip button in bottom-right corner during intro
    skipButton.setBounds(area.getWidth() - 110, area.getHeight() - 48, 90, 32);

    // Replay button in bottom-left corner (preview mode)
    replayButton.setBounds(20, area.getHeight() - 48, 110, 32);

    if (currentState == State::NameInputScreen)
    {
        int cardWidth = 480;
        int cardHeight = 390;
        int cardY = cy - (cardHeight / 2) + 30;
        auto cardBounds = juce::Rectangle<int>(cx - (cardWidth / 2), cardY, cardWidth, cardHeight);

        // Content layout inside card
        int paddingX = 36;
        int contentW = cardBounds.getWidth() - paddingX * 2;
        int startX = cardBounds.getX() + paddingX;

        // Avatar orb is at top: cardBounds.getY() + 26, size 60. Ends at y + 86.
        // Title
        int titleY = cardBounds.getY() + 102;
        cardTitleLabel.setBounds(startX, titleY, contentW, 26);

        // Subtitle
        int subY = titleY + 28;
        cardSubtitleLabel.setBounds(startX, subY, contentW, 18);

        // Name text input field: single-line compact height (38px) with vertically centered text
        int inputY = subY + 28;
        int inputH = 38;
        nameEditor.setBounds(startX, inputY, contentW, inputH);
        int topPad = (inputH - 17) / 2; // 10px
        nameEditor.setIndents(14, topPad);

        // Role Selection Chips row
        int chipY = inputY + inputH + 16;
        int chipH = 34;
        int totalChips = (int)roleButtons.size();
        int chipSpacing = 8;
        int chipW = (contentW - (totalChips - 1) * chipSpacing) / totalChips;

        for (int i = 0; i < totalChips; ++i)
        {
            roleButtons[i]->setBounds(startX + i * (chipW + chipSpacing), chipY, chipW, chipH);
        }

        // Primary Enter Button
        int btnY = chipY + chipH + 20;
        int btnH = 46;
        enterButton.setBounds(startX, btnY, contentW, btnH);
    }
}

juce::Image IntroScreenComponent::captureFrameAtTime(float timestampSeconds, int width, int height)
{
    juce::Image screenshot(juce::Image::ARGB, width, height, true);
    juce::Graphics g(screenshot);

    setVisible(true);
    setBounds(0, 0, width, height);

    totalIntroDuration = ThemeConfig::kIntroTotalDurationFirstRun;

    // Set state based on timestamp
    if (timestampSeconds < totalIntroDuration)
    {
        currentState = State::IntroAnimation;
        elapsedTime = timestampSeconds;

        cardTitleLabel.setVisible(false);
        cardSubtitleLabel.setVisible(false);
        nameEditor.setVisible(false);
        for (auto& btn : roleButtons)
            btn->setVisible(false);
        enterButton.setVisible(false);
        skipButton.setVisible(true);
    }
    else
    {
        currentState = State::NameInputScreen;
        elapsedTime = timestampSeconds;

        skipButton.setVisible(false);
        cardTitleLabel.setVisible(true);
        cardSubtitleLabel.setVisible(true);
        nameEditor.setVisible(true);
        for (auto& btn : roleButtons)
            btn->setVisible(true);
        enterButton.setVisible(true);
    }

    resized();
    paintEntireComponent(g, true);
    return screenshot;
}

} // namespace paasphoos
