#pragma once

#include <juce_core/juce_core.h>

namespace paasphoos
{

struct NetworkAdapterInfo
{
    bool isWiFi = false;
    bool isEthernet = false;
    juce::String adapterName;
    juce::String ipAddress;
    juce::String warningMessage;
};

class NetworkAdapterDetector
{
public:
    static NetworkAdapterInfo detectPrimaryAdapter();
    static bool isClientIsolationLikely(bool hasPeers, int beaconsSent, int beaconsReceived);
};

} // namespace paasphoos
