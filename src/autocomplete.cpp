#include "autocomplete.hpp"
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
const std::string AutoMatch::exactMatch() {
    if (matchType == Match::NoMatch || matchType == Match::MultipleMatch) {
        return "";
    }
    return matches.at(0);
}
AutoMatch::AutoMatch(const std::vector<std::string>& choices, std::string_view substr) {
    std::copy_if(choices.begin(), choices.end(), std::back_inserter(matches),
                 [substr](const std::string_view str) { return str.starts_with(substr); });
    switch (matches.size()) {
        case 0:
            matchType = Match::NoMatch;
            break;
        case 1:
            matchType = Match::ExactMatch;
            break;
        default:
            matchType = Match::MultipleMatch;
            break;
    }
}
