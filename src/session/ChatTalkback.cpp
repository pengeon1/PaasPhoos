#include "ChatTalkback.h"

namespace paasphoos
{

ChatTalkback::ChatTalkback()
{
    addMessage("System", "Welcome to PaasPhoos Rehearsal Room. Text chat and PTT ready.");
}

void ChatTalkback::addMessage(const juce::String& sender, const juce::String& text)
{
    if (text.trim().isEmpty()) return;

    ChatMessage msg;
    msg.sender = sender.isNotEmpty() ? sender : "Band Member";
    msg.text = text.trim();
    msg.timeString = juce::Time::getCurrentTime().formatted("%H:%M");
    messages.push_back(msg);

    if (messages.size() > 200)
        messages.erase(messages.begin());

    sendChangeMessage();
}

const ChatMessage* ChatTalkback::getMessage(int index) const noexcept
{
    if (index >= 0 && index < static_cast<int>(messages.size()))
        return &messages[index];
    return nullptr;
}

void ChatTalkback::clear()
{
    messages.clear();
    sendChangeMessage();
}

} // namespace paasphoos
