#pragma once

#include "config.h"
#include "scanner.h"
#include <random>
#include <vector>

class Playlist {
public:
    void Clear();
    void Add(const TrackEntry& entry);
    void SetItems(std::vector<TrackEntry> items);

    const std::vector<TrackEntry>& Items() const;

    int CurrentIndex() const;
    void SetCurrent(int index);

    int SelectedIndex() const;
    void SetSelected(int index);

    int NextIndex(RepeatMode repeat, bool shuffle = false) const;
    int PrevIndex(RepeatMode repeat) const;

    int FindIndexByPath(const std::filesystem::path& path) const;
    void Sort(SortMode mode);

private:
    int RandomIndex(int exclude) const;

    std::vector<TrackEntry> items_;
    int current_ = -1;
    int selected_ = -1;
};
