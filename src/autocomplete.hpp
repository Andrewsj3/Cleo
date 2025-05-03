#pragma once

#include <string>
#include <string_view>
#include <vector>
enum class Match { NoMatch, ExactMatch, MultipleMatch };

Match autocomplete(const std::vector<std::string>&, std::string_view, std::string&);
