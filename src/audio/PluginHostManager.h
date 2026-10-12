#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <atomic>
#include <vector>

namespace paasphoos
{

class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow(const juce::String& name,
                       juce::AudioProcessorEditor* editor,
                       std::function<void()> onCloseCallback)
        : juce::DocumentWindow(name, juce::Colour(0xff181a20), juce::DocumentWindow::closeButton),
          onClose(onCloseCallback)
    {
        setContentOwned(editor, true);
        setUsingNativeTitleBar(true);
        setResizable(editor->isResizable(), false);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        if (onClose) onClose();
    }

private:
    std::function<void()> onClose;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

class PluginHostManager : public juce::ChangeBroadcaster
{
public:
    PluginHostManager();
    ~PluginHostManager() override;

    void initialize(double sampleRate, int blockSize);
    void scanPlugins(const juce::String& customPath = {});
    const juce::KnownPluginList& getPluginList() const noexcept { return knownPluginList; }

    bool loadPlugin(int pluginIndex);
    void unloadPlugin();
    bool isPluginLoaded() const noexcept { return activePlugin != nullptr; }
    juce::String getLoadedPluginName() const;

    void openPluginWindow();
    void closePluginWindow();

    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    void setBypass(bool bypass) noexcept { isBypassed.store(bypass, std::memory_order_relaxed); }
    bool getBypass() const noexcept { return isBypassed.load(std::memory_order_relaxed); }

private:
    double currentSampleRate{48000.0};
    int currentBlockSize{128};

    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;

    std::unique_ptr<juce::AudioPluginInstance> activePlugin;
    std::unique_ptr<PluginEditorWindow> pluginWindow;

    std::atomic<bool> isBypassed{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHostManager)
};

} // namespace paasphoos
