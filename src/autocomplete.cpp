#include "autocomplete.hpp"
#include <algorithm>
#include <vector>

Match autocomplete(const std::vector<std::string>& choices, std::string_view substr,
                   std::string& exactMatch, std::vector<std::string>& multipleMatches) {

    std::vector<std::string> matches{};
    std::copy_if(choices.begin(), choices.end(), std::back_inserter(matches),
                 [substr](const std::string_view str) { return str.starts_with(substr); });
    if (matches.size() == 0) {
        return Match::NoMatch;
    } else if (matches.size() == 1) {
        exactMatch = matches.at(0);
        return Match::ExactMatch;
    } else {
		multipleMatches = matches;
        return Match::MultipleMatch;
    }
}
