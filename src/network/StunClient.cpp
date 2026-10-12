#include "StunClient.h"
#include <random>

namespace paasphoos
{

StunResult StunClient::resolvePublicEndpoint(int localBoundPort,
                                            const juce::String& stunServer,
                                            int stunPort)
{
    StunResult result;

    juce::DatagramSocket socket(false);
    if (!socket.bindToPort(0))
    {
        result.errorMessage = "Could not bind UDP socket for STUN query";
        return result;
    }

    // RFC 5389 STUN Binding Request (20 bytes)
    uint8_t request[20];
    request[0] = 0x00; request[1] = 0x01; // Type: 0x0001 (Binding Request)
    request[2] = 0x00; request[3] = 0x00; // Length: 0
    request[4] = 0x21; request[5] = 0x12; request[6] = 0xA4; request[7] = 0x42; // Magic Cookie

    // 12-byte Transaction ID
    std::mt19937 rng(12345);
    for (int i = 8; i < 20; ++i)
        request[i] = static_cast<uint8_t>(rng() & 0xFF);

    int sent = socket.write(stunServer, stunPort, request, sizeof(request));
    if (sent <= 0)
    {
        // Try fallback IP directly (Google STUN: 74.125.250.129)
        sent = socket.write("74.125.250.129", stunPort, request, sizeof(request));
    }

    if (sent <= 0)
    {
        result.errorMessage = "Failed to transmit STUN request packet";
        return result;
    }

    uint8_t response[512];
    juce::String senderIp;
    int senderPort = 0;

    // Wait up to 800ms for response; do NOT block indefinitely on UDP socket
    int ready = socket.waitUntilReady(true, 800);
    int bytesRead = 0;
    if (ready > 0)
    {
        // Must pass blockUntilSpecifiedAmountHasArrived = false for datagrams!
        bytesRead = socket.read(response, sizeof(response), false, senderIp, senderPort);
    }

    if (bytesRead >= 20)
    {
        uint16_t msgType = (response[0] << 8) | response[1];
        if (msgType == 0x0101) // Binding Success Response
        {
            int idx = 20;
            while (idx + 4 <= bytesRead)
            {
                uint16_t attrType = (response[idx] << 8) | response[idx + 1];
                uint16_t attrLen = (response[idx + 2] << 8) | response[idx + 3];
                idx += 4;

                if (attrType == 0x0020 && attrLen >= 8) // XOR-MAPPED-ADDRESS
                {
                    uint16_t xorPort = (response[idx + 2] << 8) | response[idx + 3];
                    int pubPort = xorPort ^ 0x2112;

                    uint32_t xorIp = (response[idx + 4] << 24) | (response[idx + 5] << 16) | (response[idx + 6] << 8) | response[idx + 7];
                    uint32_t pubIpInt = xorIp ^ 0x2112A442;

                    result.publicIp = juce::String::formatted("%d.%d.%d.%d",
                                                              (pubIpInt >> 24) & 0xFF,
                                                              (pubIpInt >> 16) & 0xFF,
                                                              (pubIpInt >> 8) & 0xFF,
                                                              pubIpInt & 0xFF);
                    result.publicPort = (localBoundPort > 0) ? localBoundPort : pubPort;
                    result.success = true;
                    result.roomCode = generateRoomCode(result.publicIp, result.publicPort);
                    return result;
                }
                else if (attrType == 0x0001 && attrLen >= 8) // MAPPED-ADDRESS
                {
                    int pubPort = (response[idx + 2] << 8) | response[idx + 3];
                    result.publicIp = juce::String::formatted("%d.%d.%d.%d", response[idx + 4], response[idx + 5], response[idx + 6], response[idx + 7]);
                    result.publicPort = (localBoundPort > 0) ? localBoundPort : pubPort;
                    result.success = true;
                    result.roomCode = generateRoomCode(result.publicIp, result.publicPort);
                    return result;
                }

                idx += attrLen;
            }
        }
    }

    // If STUN timed out (e.g. offline/isolated network), fallback to detected local IP
    result.publicIp = "127.0.0.1";
    result.publicPort = (localBoundPort > 0) ? localBoundPort : 44045;
    result.roomCode = generateRoomCode(result.publicIp, result.publicPort);
    result.success = false;
    result.errorMessage = "STUN server did not reply in time (fallback code generated)";
    return result;
}

juce::String StunClient::generateRoomCode(const juce::String& ip, int port)
{
    juce::String raw = ip + ":" + juce::String(port);
    return juce::String("PP-") + juce::Base64::toBase64(raw.toRawUTF8(), raw.getNumBytesAsUTF8());
}

bool StunClient::parseRoomCode(const juce::String& roomCode, juce::String& outIp, int& outPort)
{
    juce::String code = roomCode.trim();
    if (code.startsWithIgnoreCase("PP-"))
        code = code.substring(3);

    juce::MemoryOutputStream decoded;
    if (juce::Base64::convertFromBase64(decoded, code))
    {
        juce::String s = decoded.toString();
        int colonIdx = s.lastIndexOfChar(':');
        if (colonIdx > 0)
        {
            outIp = s.substring(0, colonIdx);
            outPort = s.substring(colonIdx + 1).getIntValue();
            return outPort > 0;
        }
    }

    // Fallback: direct ip:port string
    int colon = code.lastIndexOfChar(':');
    if (colon > 0)
    {
        outIp = code.substring(0, colon);
        outPort = code.substring(colon + 1).getIntValue();
        return outPort > 0;
    }
    return false;
}

} // namespace paasphoos
