#include "scanner.h"
#include "utils.h"

bool IsPmdFile(const std::string &name)
{
    std::string lower = utils::to_lower(name);
    size_t len = lower.size();

    // .mdt appears to be PCM samples, so not playable
    if (len >= 4 && lower.substr(len - 4) == ".mdt")
        return false;

    // .m extension
    if (len >= 2 && lower[len - 2] == '.' && lower[len - 1] == 'm')
        return true;

    // .m2 extension
    if (len >= 3 && lower[len - 3] == '.' && lower[len - 2] == 'm' && lower[len - 1] == '2')
        return true;

    // .m86 extension (PC-8801)
    if (len >= 4 && lower.substr(len - 4) == ".m86")
        return true;

    // .m26 extension (PC-8801 extended)
    if (len >= 4 && lower.substr(len - 4) == ".m26")
        return true;

    return false;
}

Scanner::Scanner() {}

Scanner::~Scanner()
{
    Stop();
}

void Scanner::Start(const std::filesystem::path &root, bool recursive, SortMode sort)
{
    Stop(); // wait for any running scan
    stop_.store(false);
    running_.store(true);
    thread_ = std::thread(&Scanner::ScanDir, this, root, recursive, sort);
}

void Scanner::Stop()
{
    stop_.store(true);
    if (thread_.joinable())
        thread_.join();
    running_.store(false);
}

bool Scanner::IsRunning() const
{
    return running_.load();
}

bool Scanner::ConsumeBatch(std::vector<TrackEntry> &out)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (batch_.empty())
        return false;
    out.swap(batch_);
    return true;
}

void Scanner::ScanDir(std::filesystem::path root, bool recursive, SortMode)
{
    std::vector<TrackEntry> local;

    auto flush = [&]()
    {
        if (local.empty())
            return;
        std::lock_guard<std::mutex> lk(mtx_);
        batch_.insert(batch_.end(), local.begin(), local.end());
        local.clear();
    };

    std::error_code ec;
    auto base = std::filesystem::weakly_canonical(root, ec);
    if (ec || !std::filesystem::exists(base))
    {
        running_.store(false);
        return;
    }

    auto process = [&](const std::filesystem::directory_entry &entry)
    {
        if (stop_.load())
            return false;

        if (!entry.is_regular_file())
            return true;

        auto p = entry.path();
        if (!IsPmdFile(p.filename().string()))
            return true;

        TrackEntry e;
        e.display_name = p.filename().string();
        e.path = std::filesystem::absolute(p, ec);
        if (ec)
        {
            e.path = p;
            ec.clear();
        }
        e.size = entry.file_size();
        e.modified = entry.last_write_time();
        local.push_back(e);

        if (local.size() >= 64)
            flush();

        return true;
    };

    if (recursive)
    {
        for (std::filesystem::recursive_directory_iterator it(base), end; it != end; ++it)
        {
            if (!process(*it))
                break;
        }
    }
    else
    {
        for (std::filesystem::directory_iterator it(base), end; it != end; ++it)
        {
            if (!process(*it))
                break;
        }
    }

    flush();
    running_.store(false);
}
