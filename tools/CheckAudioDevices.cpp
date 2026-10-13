#include <juce_core/juce_core.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <iostream>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    juce::AudioDeviceManager dm;
    dm.initialiseWithDefaultDevices(2, 2);

    std::cout << "Available Audio Device Types:" << std::endl;
    for (auto* type : dm.getAvailableDeviceTypes())
    {
        std::string typeName = type->getTypeName().toStdString();
        std::cout << "  - Driver Type: " << typeName << std::endl;
        type->scanForDevices();
        auto outDevices = type->getDeviceNames(false);
        for (const auto& dev : outDevices)
        {
            std::cout << "      Output Device: " << dev.toStdString() << std::endl;
        }
        auto inDevices = type->getDeviceNames(true);
        for (const auto& dev : inDevices)
        {
            std::cout << "      Input Device:  " << dev.toStdString() << std::endl;
        }
    }
    return 0;
}
