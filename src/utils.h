#pragma once

#include <algorithm>
#include <cctype>
#include <string>

namespace utils
{

inline std::string to_lower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return result;
}

inline bool contains_ignore_case(const std::string &haystack, const std::string &needle)
{
    if (needle.empty())
        return true;
    return to_lower(haystack).find(to_lower(needle)) != std::string::npos;
}

} // namespace utils
