#pragma once

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>

namespace paasphoos
{

struct SongItem
{
    juce::String title;
    juce::String key;
    float bpm{120.0f};
    juce::String notes;
};

class SetlistManager : public juce::ChangeBroadcaster
{
public:
    SetlistManager();
    ~SetlistManager() = default;

    int getNumSongs() const noexcept { return static_cast<int>(songs.size()); }
    const SongItem* getSong(int index) const noexcept;
    int getCurrentSongIndex() const noexcept { return currentSongIndex; }

    void selectSong(int index);
    void addSong(const juce::String& title, const juce::String& key, float bpm, const juce::String& notes = {});
    void removeSong(int index);
    void updateSong(int index, const SongItem& item);

    void saveToJson(const juce::File& file);
    void loadFromJson(const juce::File& file);

private:
    std::vector<SongItem> songs;
    int currentSongIndex{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SetlistManager)
};

} // namespace paasphoos
