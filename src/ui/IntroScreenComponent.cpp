#include "IntroScreenComponent.h"
#include <cmath>

namespace paasphoos
{

IntroScreenComponent::IntroScreenComponent()
{
    // Try to load custom logo from resources
    auto logoFile = juce::File::getCurrentWorkingDirectory().getChildFile("resources/logo.jpg");
    if (!logoFile.existsAsFile())
    {
        logoFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getChildFile("resources/logo.jpg");
    }
    if (logoFile.existsAsFile())
    {
        logoImage = juce::ImageFileFormat::loadFrom(logoFile);
    }

    // Title & Branding
    titleLabel.setText("PAASPHOOS", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffff7700));
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("ULTRA-LOW LATENCY REMOTE REHEARSAL STUDIO", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    subtitleLabel.setJustificationType(juce::Justification::centred);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    addAndMakeVisible(subtitleLabel);

    taglineLabel.setText("Real-Time Band Jamming | Zero Cloud Fees | Pure Open Source", juce::dontSendNotification);
    taglineLabel.setFont(juce::FontOptions(10.5f));
    taglineLabel.setJustificationType(juce::Justification::centred);
    taglineLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    addAndMakeVisible(taglineLabel);

    // Profile Card Container
    addAndMakeVisible(cardPanel);

    cardHeaderLabel.setText("SET YOUR MUSICIAN PROFILE", juce::dontSendNotification);
    cardHeaderLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    cardHeaderLabel.setJustificationType(juce::Justification::centred);
    cardHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffaa33));
    cardPanel.addAndMakeVisible(cardHeaderLabel);

    cardPromptLabel.setText("Enter your name and instrument to collaborate with your band:", juce::dontSendNotification);
    cardPromptLabel.setFont(juce::FontOptions(10.5f));
    cardPromptLabel.setJustificationType(juce::Justification::centred);
    cardPromptLabel.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
    cardPanel.addAndMakeVisible(cardPromptLabel);

    nameFieldLabel.setText("Musician / Stage Name:", juce::dontSendNotification);
    nameFieldLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    nameFieldLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    cardPanel.addAndMakeVisible(nameFieldLabel);

    nameEditor.setTextToShowWhenEmpty("e.g. Jimi, Freddie, Alex...", juce::Colour(0xff64748b));
    nameEditor.setFont(juce::FontOptions(13.5f, juce::Font::bold));
    nameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0a0b0f));
    nameEditor.setColour(juce::TextEditor::textColourId, juce::Colour(0xfff8fafc));
    nameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff2d2f3d));
    nameEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xffff6600));
    nameEditor.onReturnKey = [this]() { handleEnter(); };
    cardPanel.addAndMakeVisible(nameEditor);

    roleFieldLabel.setText("Instrument / Role in Band:", juce::dontSendNotification);
    roleFieldLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    roleFieldLabel.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
    cardPanel.addAndMakeVisible(roleFieldLabel);

    roleCombo.addItem("Lead Guitar", 1);
    roleCombo.addItem("Rhythm Guitar", 2);
    roleCombo.addItem("Bass Guitar", 3);
    roleCombo.addItem("Vocals / Mic", 4);
    roleCombo.addItem("Keyboards / Synth", 5);
    roleCombo.addItem("Drums / Percussion", 6);
    roleCombo.addItem("Acoustic / Aux", 7);
    roleCombo.setSelectedId(1, juce::dontSendNotification);
    cardPanel.addAndMakeVisible(roleCombo);

    enterButton.setButtonText("ENTER REHEARSAL STUDIO ->");
    enterButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6600));
    enterButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));
    enterButton.onClick = [this]() { handleEnter(); };
    cardPanel.addAndMakeVisible(enterButton);

    loadSavedProfile();
    startTimerHz(60); // 60 FPS smooth animations
}

IntroScreenComponent::~IntroScreenComponent()
{
    stopTimer();
}

void IntroScreenComponent::loadSavedProfile()
{
    auto appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("PaasPhoos");
    auto propFile = appDir.getChildFile("profile.properties");
    if (propFile.existsAsFile())
    {
        juce::PropertiesFile::Options opts;
        opts.applicationName = "PaasPhoos";
        opts.filenameSuffix = "properties";
        opts.storageFormat = juce::PropertiesFile::storeAsXML;
        juce::PropertiesFile props(propFile, opts);
        auto savedName = props.getValue("musicianName", "");
        auto savedRole = props.getIntValue("instrumentRoleId", 1);
        if (savedName.isNotEmpty())
        {
            nameEditor.setText(savedName);
        }
        if (savedRole >= 1 && savedRole <= 7)
        {
            roleCombo.setSelectedId(savedRole, juce::dontSendNotification);
        }
    }
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
    props.setValue("instrumentRoleId", roleCombo.getSelectedId());
    props.saveIfNeeded();
}

void IntroScreenComponent::show(bool animated)
{
    setVisible(true);
    toFront(true);
    isFadingOut = false;
    fadeAlpha = 1.0f;
    setAlpha(fadeAlpha);
    nameEditor.grabKeyboardFocus();
    startTimerHz(60);
}

void IntroScreenComponent::dismiss()
{
    isFadingOut = true;
}

void IntroScreenComponent::handleEnter()
{
    auto name = nameEditor.getText().trim();
    if (name.isEmpty())
        name = "Musician";

    auto role = roleCombo.getText();
    saveProfile(name, role);
    dismiss();
}

juce::String IntroScreenComponent::getMusicianName() const
{
    auto name = nameEditor.getText().trim();
    return name.isEmpty() ? "Musician" : name;
}

juce::String IntroScreenComponent::getInstrumentRole() const
{
    return roleCombo.getText();
}

void IntroScreenComponent::timerCallback()
{
    animationPhase += 0.025f;

    if (isFadingOut)
    {
        fadeAlpha -= 0.06f;
        if (fadeAlpha <= 0.0f)
        {
            fadeAlpha = 0.0f;
            stopTimer();
            setVisible(false);
            if (onStudioEntered)
            {
                onStudioEntered(getMusicianName(), getInstrumentRole());
            }
        }
        setAlpha(fadeAlpha);
    }

    repaint();
}

void IntroScreenComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Radial Dark Gradient Backdrop
    juce::ColourGradient bgGrad(juce::Colour(0xff12141c), bounds.getCentreX(), bounds.getCentreY() - 80.0f,
                               juce::Colour(0xff06070a), bounds.getCentreX(), bounds.getBottom(), true);
    bgGrad.addColour(0.4, juce::Colour(0xff0b0d12));
    g.setGradientFill(bgGrad);
    g.fillAll();

    float centerX = bounds.getCentreX();
    float logoCenterY = bounds.getCentreY() - 170.0f;

    // 2. Animated Expanding Soundwave Acoustic Ripple Rings
    for (int ring = 0; ring < 4; ++ring)
    {
        float ringProgress = std::fmod(animationPhase * 0.45f + (float)ring * 0.25f, 1.0f);
        float radius = 45.0f + (ringProgress * 180.0f);
        float alpha = std::pow(1.0f - ringProgress, 1.5f) * 0.45f;

        g.setColour(juce::Colour(0xffff6600).withAlpha(alpha));
        g.drawEllipse(centerX - radius, logoCenterY - radius, radius * 2.0f, radius * 2.0f, 1.5f + (1.0f - ringProgress));
    }

    // 3. Central Glowing Logo
    float logoSize = 80.0f;
    float pulse = 1.0f + 0.04f * std::sin(animationPhase * 2.2f);
    float scaledSize = logoSize * pulse;
    juce::Rectangle<float> logoRect(centerX - scaledSize * 0.5f, logoCenterY - scaledSize * 0.5f, scaledSize, scaledSize);

    // Ambient glow halo behind logo
    g.setColour(juce::Colour(0xffff6600).withAlpha(0.20f + 0.10f * std::sin(animationPhase * 2.0f)));
    g.fillEllipse(logoRect.expanded(18.0f));

    if (logoImage.isValid())
    {
        g.drawImage(logoImage, logoRect.getX(), logoRect.getY(), logoRect.getWidth(), logoRect.getHeight(),
                    0, 0, logoImage.getWidth(), logoImage.getHeight());
    }
    else
    {
        // Custom Glowing Vector Logo fallback
        g.setColour(juce::Colour(0xff181410));
        g.fillRoundedRectangle(logoRect, 16.0f);
        g.setColour(juce::Colour(0xffff6600));
        g.drawRoundedRectangle(logoRect, 16.0f, 2.0f);

        // Procedural headphone arc
        juce::Path arc;
        arc.addCentredArc(centerX, logoCenterY + 2.0f, 24.0f, 24.0f, 0.0f,
                          -juce::MathConstants<float>::pi * 0.75f,
                          juce::MathConstants<float>::pi * 0.75f, true);
        g.setColour(juce::Colour(0xffffaa33));
        g.strokePath(arc, juce::PathStrokeType(4.0f));
    }

    // Outer subtle border on the profile card container
    auto cardBounds = cardPanel.getBounds().toFloat();
    g.setColour(juce::Colour(0x22ff6600));
    g.fillRoundedRectangle(cardBounds.expanded(4.0f), 12.0f);

    g.setColour(juce::Colour(0xff12141d));
    g.fillRoundedRectangle(cardBounds, 10.0f);

    g.setColour(juce::Colour(0xff2d2f3d));
    g.drawRoundedRectangle(cardBounds, 10.0f, 1.0f);

    // Glowing flame-orange accent top border on card
    g.setColour(juce::Colour(0xffff6600));
    g.fillRoundedRectangle(cardBounds.getX() + 2.0f, cardBounds.getY() + 1.0f, cardBounds.getWidth() - 4.0f, 2.5f, 1.0f);
}

void IntroScreenComponent::resized()
{
    auto area = getLocalBounds();
    int centerX = area.getCentreX();

    int logoCenterY = area.getCentreY() - 170;

    // Title & Branding under the logo
    titleLabel.setBounds(centerX - 250, logoCenterY + 52, 500, 36);
    subtitleLabel.setBounds(centerX - 300, logoCenterY + 86, 600, 20);
    taglineLabel.setBounds(centerX - 300, logoCenterY + 104, 600, 18);

    // Musician Profile Card
    int cardWidth = 460;
    int cardHeight = 264;
    int cardY = logoCenterY + 130;
    cardPanel.setBounds(centerX - (cardWidth / 2), cardY, cardWidth, cardHeight);

    auto cardArea = cardPanel.getLocalBounds().reduced(20, 16);

    cardHeaderLabel.setBounds(cardArea.removeFromTop(20));
    cardPromptLabel.setBounds(cardArea.removeFromTop(18));
    cardArea.removeFromTop(10);

    nameFieldLabel.setBounds(cardArea.removeFromTop(16));
    nameEditor.setBounds(cardArea.removeFromTop(34));
    cardArea.removeFromTop(10);

    roleFieldLabel.setBounds(cardArea.removeFromTop(16));
    roleCombo.setBounds(cardArea.removeFromTop(32));
    cardArea.removeFromTop(14);

    enterButton.setBounds(cardArea.removeFromTop(38));
}

} // namespace paasphoos
