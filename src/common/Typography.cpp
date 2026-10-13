#include "Typography.h"
#include "ThemeConfig.h"
#include <BinaryData.h>

namespace paasphoos
{

namespace
{
    // Cached typefaces created once at startup - zero per-frame allocation
    juce::Typeface::Ptr gSoraBold;
    juce::Typeface::Ptr gSoraSemiBold;
    juce::Typeface::Ptr gInterRegular;
    juce::Typeface::Ptr gInterMedium;
    juce::Typeface::Ptr gInterSemiBold;
    juce::Typeface::Ptr gJetBrainsMonoRegular;
    juce::Typeface::Ptr gJetBrainsMonoMedium;

    juce::Image gLogoImage;
    bool gInitialized = false;

    juce::Typeface::Ptr loadFont(const void* data, size_t size, const juce::String& fallbackFileName, const juce::String& familyName)
    {
        juce::Typeface::Ptr tf;

        if (data != nullptr && size > 0)
        {
            tf = juce::Typeface::createSystemTypefaceFor(data, size);
            if (tf != nullptr)
                return tf;
        }

        // Disk fallback: check current working directory or executable location
        juce::File assetFile = juce::File::getCurrentWorkingDirectory().getChildFile("assets/fonts").getChildFile(fallbackFileName);
        if (!assetFile.existsAsFile())
        {
            assetFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                            .getParentDirectory().getChildFile("assets/fonts").getChildFile(fallbackFileName);
        }

        if (assetFile.existsAsFile())
        {
            juce::MemoryBlock mb;
            if (assetFile.loadFileAsData(mb) && mb.getSize() > 0)
            {
                tf = juce::Typeface::createSystemTypefaceFor(mb.getData(), mb.getSize());
                if (tf != nullptr)
                    return tf;
            }
        }

        juce::Logger::writeToLog("[WARNING] Failed to load bundled font: " + familyName + " (" + fallbackFileName + ")");
        return nullptr;
    }
}

void Typography::initialize()
{
    if (gInitialized)
        return;

    // 1. Sora (600, 700)
    gSoraSemiBold = loadFont(BinaryData::SoraSemiBold_ttf, BinaryData::SoraSemiBold_ttfSize, "Sora-SemiBold.ttf", "Sora SemiBold");
    gSoraBold     = loadFont(BinaryData::SoraBold_ttf,     BinaryData::SoraBold_ttfSize,     "Sora-Bold.ttf",     "Sora Bold");

    // 2. Inter (400, 500, 600)
    gInterRegular  = loadFont(BinaryData::InterRegular_ttf,  BinaryData::InterRegular_ttfSize,  "Inter-Regular.ttf",  "Inter Regular");
    gInterMedium   = loadFont(BinaryData::InterMedium_ttf,   BinaryData::InterMedium_ttfSize,   "Inter-Medium.ttf",   "Inter Medium");
    gInterSemiBold = loadFont(BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize, "Inter-SemiBold.ttf", "Inter SemiBold");

    // 3. JetBrains Mono (400, 500)
    gJetBrainsMonoRegular = loadFont(BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize, "JetBrainsMono-Regular.ttf", "JetBrains Mono Regular");
    gJetBrainsMonoMedium  = loadFont(BinaryData::JetBrainsMonoMedium_ttf,  BinaryData::JetBrainsMonoMedium_ttfSize,  "JetBrainsMono-Medium.ttf",  "JetBrains Mono Medium");

    // Bundled logo
    if (BinaryData::logo_jpgSize > 0)
    {
        gLogoImage = juce::ImageFileFormat::loadFrom(BinaryData::logo_jpg, (size_t)BinaryData::logo_jpgSize);
    }
    if (!gLogoImage.isValid())
    {
        auto logoFile = juce::File::getCurrentWorkingDirectory().getChildFile("resources/logo.jpg");
        if (!logoFile.existsAsFile())
        {
            logoFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
                           .getParentDirectory().getChildFile("resources/logo.jpg");
        }
        if (logoFile.existsAsFile())
        {
            gLogoImage = juce::ImageFileFormat::loadFrom(logoFile);
        }
    }

    gInitialized = true;
}

bool Typography::isInitialized()
{
    return gInitialized;
}

juce::Typeface::Ptr Typography::getSoraBoldTypeface()
{
    if (!gInitialized) initialize();
    return gSoraBold ? gSoraBold : (gInterSemiBold ? gInterSemiBold : nullptr);
}

juce::Typeface::Ptr Typography::getSoraSemiBoldTypeface()
{
    if (!gInitialized) initialize();
    return gSoraSemiBold ? gSoraSemiBold : (gSoraBold ? gSoraBold : (gInterSemiBold ? gInterSemiBold : nullptr));
}

juce::Typeface::Ptr Typography::getInterRegularTypeface()
{
    if (!gInitialized) initialize();
    return gInterRegular;
}

juce::Typeface::Ptr Typography::getInterMediumTypeface()
{
    if (!gInitialized) initialize();
    return gInterMedium ? gInterMedium : gInterRegular;
}

juce::Typeface::Ptr Typography::getInterSemiBoldTypeface()
{
    if (!gInitialized) initialize();
    return gInterSemiBold ? gInterSemiBold : (gInterMedium ? gInterMedium : gInterRegular);
}

juce::Typeface::Ptr Typography::getJetBrainsMonoRegularTypeface()
{
    if (!gInitialized) initialize();
    return gJetBrainsMonoRegular ? gJetBrainsMonoRegular : gInterRegular;
}

juce::Typeface::Ptr Typography::getJetBrainsMonoMediumTypeface()
{
    if (!gInitialized) initialize();
    return gJetBrainsMonoMedium ? gJetBrainsMonoMedium : (gJetBrainsMonoRegular ? gJetBrainsMonoRegular : gInterMedium);
}

juce::Font Typography::display(float size)
{
    auto tf = getSoraBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(size).withExtraKerningFactor(kKerningWordmark);
    return juce::Font(juce::FontOptions(size, juce::Font::bold)).withExtraKerningFactor(kKerningWordmark);
}

juce::Font Typography::h1()
{
    auto tf = getSoraBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(kSizeHeading).withExtraKerningFactor(kKerningHeading);
    return juce::Font(juce::FontOptions(kSizeHeading, juce::Font::bold)).withExtraKerningFactor(kKerningHeading);
}

juce::Font Typography::h2()
{
    auto tf = getSoraSemiBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(kSizeSection).withExtraKerningFactor(kKerningHeading);
    return juce::Font(juce::FontOptions(kSizeSection, juce::Font::bold)).withExtraKerningFactor(kKerningHeading);
}

juce::Font Typography::h3()
{
    auto tf = getSoraSemiBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(kSizeEmphasis);
    return juce::Font(juce::FontOptions(kSizeEmphasis, juce::Font::bold));
}

juce::Font Typography::body()
{
    auto tf = getInterRegularTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(kSizeBody);
    return juce::Font(juce::FontOptions(kSizeBody));
}

juce::Font Typography::bodyStrong()
{
    auto tf = getInterSemiBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(kSizeBody);
    return juce::Font(juce::FontOptions(kSizeBody, juce::Font::bold));
}

juce::Font Typography::bodySecondary()
{
    auto tf = getInterRegularTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(kSizeSecondary);
    return juce::Font(juce::FontOptions(kSizeSecondary));
}

juce::Font Typography::caption(float size)
{
    auto tf = getInterRegularTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(size);
    return juce::Font(juce::FontOptions(size));
}

juce::Font Typography::labelCaps(float size)
{
    auto tf = getInterSemiBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(size).withExtraKerningFactor(kKerningLabelCaps);
    return juce::Font(juce::FontOptions(size, juce::Font::bold)).withExtraKerningFactor(kKerningLabelCaps);
}

juce::Font Typography::numeric(float size)
{
    auto tf = getJetBrainsMonoRegularTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(size);
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
}

juce::Font Typography::numericLarge(float size)
{
    auto tf = getJetBrainsMonoMediumTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(size);
    return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::bold));
}

juce::Font Typography::button(float size)
{
    auto tf = getSoraSemiBoldTypeface();
    if (tf != nullptr)
        return juce::Font(tf).withHeight(size);
    return juce::Font(juce::FontOptions(size, juce::Font::bold));
}

void Typography::drawWordmark(juce::Graphics& g,
                              const juce::Rectangle<float>& bounds,
                              float fontSize,
                              float alpha,
                              juce::Justification justification)
{
    auto font = display(fontSize);
    g.setFont(font);

    juce::ColourGradient grad(juce::Colour(ThemeConfig::kAccentVioletHex).withAlpha(alpha),
                              bounds.getX(), bounds.getCentreY(),
                              juce::Colour(ThemeConfig::kAccentBlueHex).withAlpha(alpha),
                              bounds.getRight(), bounds.getCentreY(), false);
    g.setGradientFill(grad);
    g.drawText("PaasPhoos", bounds, justification, false);
}

juce::String Typography::truncateWithEllipsis(const juce::String& text,
                                              const juce::Font& font,
                                              float maxWidth)
{
    if (maxWidth <= 0.0f)
        return "";

    auto strWidth = font.getStringWidthFloat(text);
    if (strWidth <= maxWidth)
        return text;

    juce::String ellipsis = "...";
    auto ellipsisWidth = font.getStringWidthFloat(ellipsis);
    if (ellipsisWidth >= maxWidth)
        return ".";

    float targetWidth = maxWidth - ellipsisWidth;
    int len = text.length();
    while (len > 0 && font.getStringWidthFloat(text.substring(0, len)) > targetWidth)
    {
        --len;
    }

    return text.substring(0, len).trimEnd() + ellipsis;
}

juce::Image Typography::getLogoImage()
{
    if (!gInitialized) initialize();
    return gLogoImage;
}

juce::Typeface::Ptr Typography::resolveTypeface(const juce::Font& font)
{
    if (!gInitialized) initialize();

    // Check if the font requested a specific family or monospaced
    auto name = font.getTypefaceName().toLowerCase();
    bool isBold = font.isBold();

    if (name.contains("jetbrains") || name.contains("mono") || font.getTypefaceStyle().containsIgnoreCase("mono"))
    {
        return isBold ? getJetBrainsMonoMediumTypeface() : getJetBrainsMonoRegularTypeface();
    }
    if (name.contains("sora") || font.getHeight() >= 24.0f)
    {
        return isBold ? getSoraBoldTypeface() : getSoraSemiBoldTypeface();
    }

    // Default UI font: Inter
    if (isBold)
        return getInterSemiBoldTypeface();
    if (font.getHeight() > 14.0f)
        return getInterMediumTypeface();
    return getInterRegularTypeface();
}

// =============================================================================
// PaasPhoosWordmarkComponent
// =============================================================================
PaasPhoosWordmarkComponent::PaasPhoosWordmarkComponent(float fontSize)
    : currentFontSize(fontSize)
{
}

void PaasPhoosWordmarkComponent::setFontSize(float newSize)
{
    if (currentFontSize != newSize)
    {
        currentFontSize = newSize;
        repaint();
    }
}

void PaasPhoosWordmarkComponent::paint(juce::Graphics& g)
{
    Typography::drawWordmark(g, getLocalBounds().toFloat(), currentFontSize, 1.0f, juce::Justification::centredLeft);
}

} // namespace paasphoos
