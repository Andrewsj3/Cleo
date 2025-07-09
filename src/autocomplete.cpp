#include "autocomplete.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
const std::string MusicMatch::exactMatch() {
    if (matchType == Match::NoMatch || matchType == Match::MultipleMatch) {
        return "";
    }
    return matches.at(0);
}

MusicMatch autocomplete(const std::vector<std::string>& choices, std::string_view substr) {
    MusicMatch match{};
    std::copy_if(choices.begin(), choices.end(), std::back_inserter(match.matches),
                 [substr](const std::string_view str) { return str.starts_with(substr); });
    switch (match.matches.size()) {
    case 0:
        match.matchType = Match::NoMatch;
        break;
    case 1:
        match.matchType = Match::ExactMatch;
        break;
    default:
        match.matchType = Match::MultipleMatch;
        break;
    }
    return match;
}
