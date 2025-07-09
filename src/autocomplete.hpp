#pragma once

#include <string>
#include <string_view>
#include <vector>
enum class Match { NoMatch, ExactMatch, MultipleMatch };
struct MusicMatch {
    Match matchType{};
    std::vector<std::string> matches{};
    const std::string exactMatch();
};

MusicMatch autocomplete(const std::vector<std::string>&, std::string_view);
