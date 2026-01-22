#include "playlist.h"
#include <algorithm>

void Playlist::Clear()
{
    items_.clear();
    current_ = -1;
    selected_ = -1;
}

void Playlist::Add(const TrackEntry &entry)
{
    items_.push_back(entry);
    if (selected_ < 0)
        selected_ = 0;
}

void Playlist::SetItems(std::vector<TrackEntry> items)
{
    items_ = std::move(items);
    current_ = items_.empty() ? -1 : 0;
    selected_ = current_;
}

const std::vector<TrackEntry> &Playlist::Items() const
{
    return items_;
}
int Playlist::CurrentIndex() const
{
    return current_;
}
int Playlist::SelectedIndex() const
{
    return selected_;
}

void Playlist::SetCurrent(int idx)
{
    current_ = (idx >= 0 && idx < (int)items_.size()) ? idx : -1;
}

void Playlist::SetSelected(int idx)
{
    selected_ = (idx >= 0 && idx < (int)items_.size()) ? idx : -1;
}

int Playlist::RandomIndex(int exclude) const
{
    if (items_.empty())
        return -1;
    if (items_.size() == 1)
        return 0;

    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, (int)items_.size() - 1);

    int pick = exclude;
    while (pick == exclude)
        pick = dist(rng);
    return pick;
}

int Playlist::NextIndex(RepeatMode repeat, bool shuffle) const
{
    if (items_.empty())
        return -1;

    if (repeat == RepeatMode::One && current_ >= 0)
        return current_;

    if (shuffle)
        return RandomIndex(current_);

    int next = current_ + 1;
    if (next >= (int)items_.size())
        return (repeat == RepeatMode::All) ? 0 : -1;
    return next;
}

int Playlist::PrevIndex(RepeatMode repeat) const
{
    if (items_.empty())
        return -1;

    if (repeat == RepeatMode::One && current_ >= 0)
        return current_;

    int prev = current_ - 1;
    if (prev < 0)
        return (repeat == RepeatMode::All) ? (int)items_.size() - 1 : -1;
    return prev;
}

int Playlist::FindIndexByPath(const std::filesystem::path &path) const
{
    for (size_t i = 0; i < items_.size(); i++)
    {
        if (items_[i].path == path)
            return (int)i;
    }
    return -1;
}

void Playlist::Sort(SortMode mode)
{
    switch (mode)
    {
    case SortMode::Name:
        std::sort(items_.begin(), items_.end(),
                  [](auto &a, auto &b) { return a.display_name < b.display_name; });
        break;
    case SortMode::Date:
        std::sort(items_.begin(), items_.end(),
                  [](auto &a, auto &b) { return a.modified < b.modified; });
        break;
    case SortMode::Size:
        std::sort(items_.begin(), items_.end(), [](auto &a, auto &b) { return a.size < b.size; });
        break;
    }
}
