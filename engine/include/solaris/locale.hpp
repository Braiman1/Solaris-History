#pragma once
#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace sol {
// Translate at presentation time, so saves and event history survive language switches.
struct Locale {
    inline static std::unordered_map<std::string, std::string> cache;
    inline static bool english = false;
    inline static std::vector<std::pair<std::string, std::string>> phrases;
    static bool load(const std::string& path) {
        Json root = Json::parse(readTextFile(path));
        if (!root.isArray()) return false;
        phrases.clear(); cache.clear();
        for (const auto& item : root.array()) phrases.emplace_back(item["es"].string(), item["en"].string());
        std::stable_sort(phrases.begin(), phrases.end(), [](const auto& a, const auto& b) { return a.first.size() > b.first.size(); });
        return true;
    }
    static bool word(unsigned char c) { return std::isalnum(c) || c >= 128 || c == '_'; }
    static std::string text(const std::string& source) {
        if (!english) return source;
        auto cached = cache.find(source);
        if (cached != cache.end()) return cached->second;
        std::string out;
        for (size_t i = 0; i < source.size();) {
            bool matched = false;
            for (const auto& p : phrases) {
                if (p.first.empty() || source.compare(i, p.first.size(), p.first) != 0) continue;
                size_t end = i + p.first.size();
                if (word(p.first.front()) && i && word(source[i-1])) continue;
                if (word(p.first.back()) && end < source.size() && word(source[end])) continue;
                out += p.second; i = end; matched = true; break;
            }
            if (!matched) out += source[i++];
        }
        if (cache.size() > 4096) cache.clear();
        cache.emplace(source, out);
        return out;
    }
};
} // namespace sol
