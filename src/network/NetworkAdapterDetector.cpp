#include "NetworkAdapterDetector.h"

#if JUCE_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

namespace paasphoos
{

NetworkAdapterInfo NetworkAdapterDetector::detectPrimaryAdapter()
{
    NetworkAdapterInfo info;

#if JUCE_WINDOWS
    ULONG bufferSize = 15000;
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    ULONG flags = GAA_FLAG_INCLUDE_GATEWAYS | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    DWORD ret = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &bufferSize);

    if (ret == ERROR_BUFFER_OVERFLOW)
    {
        buffer.resize(bufferSize);
        adapters = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        ret = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &bufferSize);
    }

    if (ret == NO_ERROR)
    {
        for (PIP_ADAPTER_ADDRESSES curr = adapters; curr != nullptr; curr = curr->Next)
        {
            // Only examine adapters that are UP and have a default gateway (meaning active internet/LAN)
            if (curr->OperStatus == IfOperStatusUp && curr->FirstGatewayAddress != nullptr)
            {
                info.adapterName = juce::String(curr->FriendlyName);

                if (curr->IfType == IF_TYPE_IEEE80211)
                {
                    info.isWiFi = true;
                    info.warningMessage = "WI-FI DETECTED: Wireless jitter causes audio latency spikes. Use wired Ethernet for best rehearsal results.";
                }
                else if (curr->IfType == IF_TYPE_ETHERNET_CSMACD)
                {
                    info.isEthernet = true;
                    info.warningMessage = "";
                }

                // Retrieve IPv4 address
                for (PIP_ADAPTER_UNICAST_ADDRESS unicast = curr->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next)
                {
                    if (unicast->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(unicast->Address.lpSockaddr);
                        char ipStr[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                        info.ipAddress = juce::String(ipStr);
                        break;
                    }
                }

                if (info.isWiFi || info.isEthernet)
                    break;
            }
        }
    }
#endif

    if (info.adapterName.isEmpty())
    {
        info.adapterName = "Local Network Interface";
        info.ipAddress = "127.0.0.1";
    }

    return info;
}

bool NetworkAdapterDetector::isClientIsolationLikely(bool hasPeers, int beaconsSent, int beaconsReceived)
{
    // If a host or client has sent numerous discovery beacons but received zero responses on a multi-device subnet
    return (!hasPeers && beaconsSent >= 8 && beaconsReceived == 0);
}

} // namespace paasphoos
