#include "SetlistManager.h"

namespace paasphoos
{

SetlistManager::SetlistManager()
{
    // Populate default rehearsal setlist
    addSong("Back in Black", "E Minor", 90.0f, "Intro: E D A riff | Rhythms heavy");
    addSong("Sweet Child O' Mine", "D Major", 125.0f, "Guitar 1 intro riff | Keyboards soft pad");
    addSong("Hotel California", "B Minor", 75.0f, "12-string acoustic opening | Drum groove steady");
    addSong("Comfortably Numb", "B Minor", 64.0f, "Vocalist clean verse | High gain solo at 4:30");
    addSong("Gallan Goodiyan", "C Major", 128.0f, "Dholak percussion intro | Full band energetic rhythm");
}

const SongItem* SetlistManager::getSong(int index) const noexcept
{
    if (index >= 0 && index < static_cast<int>(songs.size()))
        return &songs[index];
    return nullptr;
}

void SetlistManager::selectSong(int index)
{
    if (index >= 0 && index < static_cast<int>(songs.size()))
    {
        currentSongIndex = index;
        sendChangeMessage();
    }
}

void SetlistManager::addSong(const juce::String& title, const juce::String& key, float bpm, const juce::String& notes)
{
    SongItem item;
    item.title = title.isNotEmpty() ? title : "Untitled Song";
    item.key = key.isNotEmpty() ? key : "C";
    item.bpm = juce::jlimit(40.0f, 300.0f, bpm);
    item.notes = notes;
    songs.push_back(item);
    sendChangeMessage();
}

void SetlistManager::removeSong(int index)
{
    if (index >= 0 && index < static_cast<int>(songs.size()))
    {
        songs.erase(songs.begin() + index);
        if (currentSongIndex >= static_cast<int>(songs.size()))
            currentSongIndex = std::max(0, static_cast<int>(songs.size()) - 1);
        sendChangeMessage();
    }
}

void SetlistManager::updateSong(int index, const SongItem& item)
{
    if (index >= 0 && index < static_cast<int>(songs.size()))
    {
        songs[index] = item;
        sendChangeMessage();
    }
}

void SetlistManager::saveToJson(const juce::File& file)
{
    juce::var root(new juce::DynamicObject());
    juce::Array<juce::var> songArray;

    for (const auto& s : songs)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("title", s.title);
        obj->setProperty("key", s.key);
        obj->setProperty("bpm", s.bpm);
        obj->setProperty("notes", s.notes);
        songArray.add(juce::var(obj));
    }

    root.getDynamicObject()->setProperty("songs", songArray);
    file.replaceWithText(juce::JSON::toString(root));
}

void SetlistManager::loadFromJson(const juce::File& file)
{
    if (!file.existsAsFile()) return;

    auto parsed = juce::JSON::parse(file);
    if (parsed.isObject() && parsed.hasProperty("songs"))
    {
        auto* arr = parsed["songs"].getArray();
        if (arr != nullptr)
        {
            songs.clear();
            for (const auto& elem : *arr)
            {
                SongItem item;
                item.title = elem.getProperty("title", "Untitled").toString();
                item.key = elem.getProperty("key", "C").toString();
                item.bpm = static_cast<float>(elem.getProperty("bpm", 120.0f));
                item.notes = elem.getProperty("notes", "").toString();
                songs.push_back(item);
            }
            currentSongIndex = 0;
            sendChangeMessage();
        }
    }
}

} // namespace paasphoos
