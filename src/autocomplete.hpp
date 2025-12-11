#pragma once

#include <string>
#include <string_view>
#include <vector>
enum class Match { NoMatch, ExactMatch, MultipleMatch };
struct AutoMatch {
    AutoMatch(const std::vector<std::string>& choices, std::string_view substr);
    Match matchType{};
    std::vector<std::string> matches{};
    const std::string exactMatch() const;
};
