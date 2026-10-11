#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace paasphoos
{

class DarkStudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkStudioLookAndFeel()
    {
        // Dark Studio Color Scheme
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff121316));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff242730));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff06b6d4));
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xfff1f5f9));
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff0f172a));

        setColour(juce::Slider::thumbColourId, juce::Colour(0xff06b6d4));
        setColour(juce::Slider::trackColourId, juce::Colour(0xff334155));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff06b6d4));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1e293b));

        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1e222b));
        setColour(juce::ComboBox::textColourId, juce::Colour(0xfff8fafc));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff334155));
        setColour(juce::ComboBox::arrowColourId, juce::Colour(0xff06b6d4));

        setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff181a20));
        setColour(juce::PopupMenu::textColourId, juce::Colour(0xfff1f5f9));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff06b6d4));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff0f172a));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = juce::jmin(6.0f, radius * 0.35f);
        auto arcRadius = radius - lineW * 0.5f;
        auto center = bounds.getCentre();

        // Background track arc
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(center.x, center.y, arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff1e293b));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active value arc
        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc(center.x, center.y, arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(slider.findColour(juce::Slider::rotarySliderFillColourId));
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner knob dial
        auto innerRadius = radius - lineW * 1.5f;
        if (innerRadius > 4.0f)
        {
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2d3342), center.x, center.y - innerRadius,
                                                   juce::Colour(0xff181b22), center.x, center.y + innerRadius, false));
            g.fillEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f);

            g.setColour(juce::Colour(0xff475569));
            g.drawEllipse(center.x - innerRadius, center.y - innerRadius, innerRadius * 2.0f, innerRadius * 2.0f, 1.0f);

            // Pointer tick
            juce::Path dialTick;
            dialTick.addRectangle(-1.5f, -innerRadius + 2.0f, 3.0f, innerRadius * 0.5f);
            g.setColour(juce::Colour(0xfff8fafc));
            g.fillPath(dialTick, juce::AffineTransform::rotation(toAngle).translated(center.x, center.y));
        }
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 6.0f;
            auto trackX = x + (width - trackWidth) * 0.5f;
            auto trackBounds = juce::Rectangle<float>(trackX, (float)y + 6.0f, trackWidth, (float)height - 12.0f);

            // Background slot
            g.setColour(juce::Colour(0xff1e293b));
            g.fillRoundedRectangle(trackBounds, 3.0f);

            // Filled active track from bottom to slider thumb
            auto activeBounds = juce::Rectangle<float>(trackX, sliderPos, trackWidth, (float)y + height - 6.0f - sliderPos);
            g.setColour(slider.findColour(juce::Slider::thumbColourId).withAlpha(0.7f));
            g.fillRoundedRectangle(activeBounds, 3.0f);

            // Fader handle thumb
            auto thumbHeight = 16.0f;
            auto thumbWidth = juce::jmin((float)width - 4.0f, 26.0f);
            auto thumbX = x + (width - thumbWidth) * 0.5f;
            auto thumbY = sliderPos - thumbHeight * 0.5f;

            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff475569), thumbX, thumbY,
                                                   juce::Colour(0xff1e293b), thumbX, thumbY + thumbHeight, false));
            g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);

            // Thumb center accent line
            g.setColour(slider.findColour(juce::Slider::thumbColourId));
            g.fillRect(thumbX + 4.0f, sliderPos - 1.0f, thumbWidth - 8.0f, 2.0f);

            g.setColour(juce::Colour(0xff64748b));
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
            baseColor = baseColor.brighter(0.15f);

        g.setColour(baseColor);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(button.getToggleState() ? baseColor.brighter(0.3f) : juce::Colour(0xff334155));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }
};

} // namespace paasphoos
