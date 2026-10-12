#include "PluginHostManager.h"

namespace paasphoos
{

PluginHostManager::PluginHostManager()
{
    formatManager.addDefaultFormats();
}

PluginHostManager::~PluginHostManager()
{
    unloadPlugin();
}

void PluginHostManager::initialize(double sampleRate, int blockSize)
{
    currentSampleRate = (sampleRate > 1000.0) ? sampleRate : 48000.0;
    currentBlockSize = std::max(64, blockSize);

    if (activePlugin != nullptr)
    {
        activePlugin->prepareToPlay(currentSampleRate, currentBlockSize);
    }
}

void PluginHostManager::scanPlugins(const juce::String& customPath)
{
    juce::FileSearchPath searchPath;

    // Standard Windows VST3 path
    searchPath.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    searchPath.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));

    if (customPath.isNotEmpty())
    {
        searchPath.add(juce::File(customPath));
    }

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format != nullptr)
        {
            juce::PluginDirectoryScanner scanner(
                knownPluginList,
                *format,
                searchPath,
                true, // recursive
                juce::File()
            );

            juce::String name;
            while (scanner.scanNextFile(true, name))
            {
                // Scanning in background / synchronous
            }
        }
    }

    sendChangeMessage();
}

bool PluginHostManager::loadPlugin(int pluginIndex)
{
    unloadPlugin();

    if (pluginIndex < 0 || pluginIndex >= knownPluginList.getNumTypes())
        return false;

    auto desc = knownPluginList.getType(pluginIndex);
    juce::String errorMessage;

    activePlugin = formatManager.createPluginInstance(
        *desc,
        currentSampleRate,
        currentBlockSize,
        errorMessage
    );

    if (activePlugin != nullptr)
    {
        activePlugin->prepareToPlay(currentSampleRate, currentBlockSize);
        sendChangeMessage();
        return true;
    }

    DBG("Failed to load VST3 plugin: " + errorMessage);
    sendChangeMessage();
    return false;
}

void PluginHostManager::unloadPlugin()
{
    closePluginWindow();
    if (activePlugin != nullptr)
    {
        activePlugin->releaseResources();
        activePlugin.reset();
    }
    sendChangeMessage();
}

juce::String PluginHostManager::getLoadedPluginName() const
{
    if (activePlugin != nullptr)
        return activePlugin->getName();
    return "None (Using Built-in DSP)";
}

void PluginHostManager::openPluginWindow()
{
    if (activePlugin == nullptr)
        return;

    if (pluginWindow != nullptr)
    {
        pluginWindow->toFront(true);
        return;
    }

    if (auto* editor = activePlugin->createEditorIfNeeded())
    {
        pluginWindow = std::make_unique<PluginEditorWindow>(
            activePlugin->getName(),
            editor,
            [this]() { closePluginWindow(); }
        );
    }
}

void PluginHostManager::closePluginWindow()
{
    pluginWindow.reset();
}

void PluginHostManager::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (isBypassed.load(std::memory_order_relaxed))
        return;

    if (activePlugin != nullptr)
    {
        activePlugin->processBlock(buffer, midiMessages);
    }
}

} // namespace paasphoos
