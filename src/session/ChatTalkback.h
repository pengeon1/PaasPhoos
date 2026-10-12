#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>
#include <atomic>

namespace paasphoos
{

struct ChatMessage
{
    juce::String sender;
    juce::String text;
    juce::String timeString;
};

class ChatTalkback : public juce::ChangeBroadcaster
{
public:
    ChatTalkback();
    ~ChatTalkback() = default;

    void addMessage(const juce::String& sender, const juce::String& text);
    int getNumMessages() const noexcept { return static_cast<int>(messages.size()); }
    const ChatMessage* getMessage(int index) const noexcept;

    // Push-to-Talk (PTT)
    void setPushToTalkActive(bool active) noexcept { pttActive.store(active, std::memory_order_relaxed); }
    bool isPushToTalkActive() const noexcept { return pttActive.load(std::memory_order_relaxed); }

    void clear();

private:
    std::vector<ChatMessage> messages;
    std::atomic<bool> pttActive{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatTalkback)
};

} // namespace paasphoos
