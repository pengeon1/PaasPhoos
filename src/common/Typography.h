#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

namespace paasphoos
{

/**
 * Centralized Typography & Design Token System for PaasPhoos.
 *
 * Rules:
 *  - Sora: logo/wordmark, screen titles, big headings, tagline, primary buttons.
 *  - Inter: all UI text (labels, menus, chat, settings, tooltips, body copy, setlist, channel names).
 *  - JetBrains Mono: all live numeric readouts (ping, jitter, packet loss, latency budget in ms,
 *                    BPM, dB meters, time codes, sample rate, buffer size). Tabular figures that never jitter.
 *  - No Arial, Segoe UI, Roboto, Helvetica, or system default font for visible text anywhere.
 *  - Cached Typeface objects created once at startup. No per-frame font creation.
 */
class Typography
{
public:
    // =========================================================================
    // Font Scales (px at 100% DPI)
    // =========================================================================
    static constexpr float kSizeCaption        = 12.0f; // captions, meter labels
    static constexpr float kSizeSecondary      = 13.0f; // secondary UI
    static constexpr float kSizeBody           = 14.0f; // default UI
    static constexpr float kSizeEmphasis       = 16.0f; // h3, emphasis
    static constexpr float kSizeSection        = 20.0f; // h2, section titles
    static constexpr float kSizeHeading        = 28.0f; // h1, screen headings
    static constexpr float kSizeDisplay        = 44.0f; // hero text / wordmark
    static constexpr float kSizeDisplayLarge   = 56.0f; // large wordmark

    // =========================================================================
    // Kerning / Letter-spacing Tokens
    // =========================================================================
    static constexpr float kKerningWordmark    = -0.030f; // -0.03em tight wordmark
    static constexpr float kKerningHeading     = -0.025f; // -0.02em to -0.03em on Sora headings >= 28px
    static constexpr float kKerningBody        =  0.000f; // 0 on Inter body text
    static constexpr float kKerningLabelCaps   =  0.060f; // +0.04em to +0.08em on small ALL-CAPS labels

    // =========================================================================
    // Core Initialization & Cache
    // =========================================================================
    static void initialize();
    static bool isInitialized();

    // Typeface accessors (cached juce::Typeface::Ptr)
    static juce::Typeface::Ptr getSoraBoldTypeface();
    static juce::Typeface::Ptr getSoraSemiBoldTypeface();
    static juce::Typeface::Ptr getInterRegularTypeface();
    static juce::Typeface::Ptr getInterMediumTypeface();
    static juce::Typeface::Ptr getInterSemiBoldTypeface();
    static juce::Typeface::Ptr getJetBrainsMonoRegularTypeface();
    static juce::Typeface::Ptr getJetBrainsMonoMediumTypeface();

    // =========================================================================
    // Named Text Styles
    // =========================================================================
    static juce::Font display(float size = kSizeDisplay);
    static juce::Font h1();
    static juce::Font h2();
    static juce::Font h3();
    static juce::Font body();
    static juce::Font bodyStrong();
    static juce::Font bodySecondary();
    static juce::Font caption(float size = kSizeCaption);
    static juce::Font labelCaps(float size = 11.0f);
    static juce::Font numeric(float size = kSizeSecondary);
    static juce::Font numericLarge(float size = 18.0f);
    static juce::Font button(float size = kSizeBody);

    // =========================================================================
    // Wordmark Rendering & Reusable Component
    // =========================================================================
    static void drawWordmark(juce::Graphics& g,
                             const juce::Rectangle<float>& bounds,
                             float fontSize = 38.0f,
                             float alpha = 1.0f,
                             juce::Justification justification = juce::Justification::centred);

    // Truncate text with ellipsis if it exceeds maxWidth
    static juce::String truncateWithEllipsis(const juce::String& text,
                                             const juce::Font& font,
                                             float maxWidth);

    // Bundled application logo image loader
    static juce::Image getLogoImage();

    // LookAndFeel typeface resolver
    static juce::Typeface::Ptr resolveTypeface(const juce::Font& font);
};

/**
 * Reusable PaasPhoos Wordmark Component
 * Renders the brand wordmark with Sora 700 and vibrant gradient fill.
 */
class PaasPhoosWordmarkComponent : public juce::Component
{
public:
    explicit PaasPhoosWordmarkComponent(float fontSize = 36.0f);
    void setFontSize(float newSize);
    void paint(juce::Graphics& g) override;

private:
    float currentFontSize{36.0f};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PaasPhoosWordmarkComponent)
};

} // namespace paasphoos
