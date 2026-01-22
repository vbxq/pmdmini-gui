#pragma once

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct TrackEntry
{
    std::string display_name;
    std::filesystem::path path;
    uintmax_t size = 0;
    std::filesystem::file_time_type modified;
};

enum class SortMode
{
    Name,
    Date,
    Size
};

bool IsPmdFile(const std::string &name);

class Scanner
{
  public:
    Scanner();
    ~Scanner();

    void Start(const std::filesystem::path &root, bool recursive, SortMode sort);
    void Stop();
    bool IsRunning() const;

    bool ConsumeBatch(std::vector<TrackEntry> &out);

  private:
    void ScanDir(std::filesystem::path root, bool recursive, SortMode sort);

    std::atomic<bool> running_{false};
    std::atomic<bool> stop_{false};
    std::thread thread_;
    std::mutex mtx_;
    std::vector<TrackEntry> batch_;
};
