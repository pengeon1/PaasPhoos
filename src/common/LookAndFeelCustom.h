#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeConfig.h"

namespace paasphoos
{

/**
 * State-of-the-Art Studio Dark LookAndFeel for PaasPhoos.
 * Consistent global design system:
 * Base: #0B0D12, Surface: #12151C, Elevated: #1A1E28
 * Accents: Violet #7C5CFF -> Electric Blue #3DA5FF -> Cyan #38E1C6
 */
class DarkStudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkStudioLookAndFeel()
    {
        // Window & Backgrounds
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(ThemeConfig::kBaseBackgroundHex));

        // Buttons
        setColour(juce::TextButton::buttonColourId, juce::Colour(ThemeConfig::kSurfaceElevatedHex));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(ThemeConfig::kAccentVioletHex));
        setColour(juce::TextButton::textColourOffId, juce::Colour(ThemeConfig::kTextPrimaryHex));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xffffffff));

        // Sliders
        setColour(juce::Slider::thumbColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
        setColour(juce::Slider::trackColourId, juce::Colour(ThemeConfig::kSurfaceHex));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(ThemeConfig::kAccentVioletHex));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));

        // Combo Boxes
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(ThemeConfig::kSurfaceHex));
        setColour(juce::ComboBox::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(ThemeConfig::kAccentBlueHex));

        // Labels & Menus
        setColour(juce::Label::textColourId, juce::Colour(ThemeConfig::kTextMutedHex));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(ThemeConfig::kSurfaceHex));
        setColour(juce::PopupMenu::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(ThemeConfig::kAccentVioletHex));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffffffff));

        // Text Editors
        setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0e1118));
        setColour(juce::TextEditor::textColourId, juce::Colour(ThemeConfig::kTextPrimaryHex));
        setColour(juce::TextEditor::outlineColourId, juce::Colour(ThemeConfig::kBorderSubtleHex));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(ThemeConfig::kAccentBlueHex));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin(5.0f, radius * 0.30f);
        auto arcRadius = radius - lineW * 0.5f;
        auto center = bounds.getCentre();

        // Background track arc
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(center.x, center.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(ThemeConfig::kBorderSubtleHex));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active value arc with smooth gradient
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(center.x, center.y, arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, toAngle, true);

            juce::ColourGradient grad(juce::Colour(ThemeConfig::kAccentVioletHex), center.x - arcRadius, center.y,
                                      juce::Colour(ThemeConfig::kAccentBlueHex), center.x + arcRadius, center.y, false);
            g.setGradientFill(grad);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner knob dial
        auto innerRadius = radius - lineW * 1.5f;
        if (innerRadius > 4.0f)
        {
            juce::ColourGradient dialGrad(juce::Colour(ThemeConfig::kSurfaceElevatedHex), center.x, center.y - innerRadius,
                                         juce::Colour(ThemeConfig::kSurfaceHex), center.x, center.y + innerRadius, false);
            g.setGradientFill(dialGrad);
            g.fillEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

            g.setColour(juce::Colour(ThemeConfig::kBorderHighlightHex));
            g.drawEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

            // Pointer tick
            juce::Path dialTick;
            dialTick.addRectangle(-1.2f, -innerRadius + 2.0f, 2.4f, innerRadius * 0.45f);
            g.setColour(juce::Colour(ThemeConfig::kAccentCyanHex));
            g.fillPath(dialTick, juce::AffineTransform::rotation(toAngle).translated(center.x, center.y));
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 5.0f;
            auto trackX = x + (width - trackWidth) * 0.5f;
            auto trackBounds = juce::Rectangle<float>(trackX, (float)y + 6.0f, trackWidth, (float)height - 12.0f);

            // Background slot
            g.setColour(juce::Colour(ThemeConfig::kSurfaceHex));
            g.fillRoundedRectangle(trackBounds, 2.5f);

            // Filled active track
            auto activeBounds = juce::Rectangle<float>(trackX, sliderPos, trackWidth, (float)y + height - 6.0f - sliderPos);
            juce::ColourGradient activeGrad(juce::Colour(ThemeConfig::kAccentBlueHex), trackX, (float)y + height,
                                           juce::Colour(ThemeConfig::kAccentVioletHex), trackX, sliderPos, false);
            g.setGradientFill(activeGrad);
            g.fillRoundedRectangle(activeBounds, 2.5f);

            // Fader thumb
            auto thumbHeight = 16.0f;
            auto thumbWidth = juce::jmin((float)width - 4.0f, 24.0f);
            auto thumbX = x + (width - thumbWidth) * 0.5f;
            auto thumbY = sliderPos - thumbHeight * 0.5f;

            juce::ColourGradient thumbGrad(juce::Colour(ThemeConfig::kSurfaceElevatedHex), thumbX, thumbY,
                                          juce::Colour(ThemeConfig::kSurfaceHex), thumbX, thumbY + thumbHeight, false);
            g.setGradientFill(thumbGrad);
            g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);

            // Thumb center accent notch
            g.setColour(juce::Colour(ThemeConfig::kAccentBlueHex));
            g.fillRect(thumbX + 4.0f, sliderPos - 1.0f, thumbWidth - 8.0f, 2.0f);

            g.setColour(juce::Colour(ThemeConfig::kBorderHighlightHex));
            g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f, 1.0f);
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto baseColor = backgroundColour;

        if (button.getToggleState())
            baseColor = button.findColour(juce::TextButton::buttonOnColourId);
        else if (isButtonDown)
            baseColor = baseColor.darker(0.2f);
        else if (isMouseOverButton)
            baseColor = baseColor.brighter(0.12f);

        g.setColour(baseColor);
        g.fillRoundedRectangle(bounds, 6.0f);

        g.setColour(button.getToggleState() ? juce::Colour(ThemeConfig::kAccentCyanHex) : juce::Colour(ThemeConfig::kBorderSubtleHex));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }
};

} // namespace paasphoos
