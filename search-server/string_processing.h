#pragma once
#include <vector>
#include <string_view>
#include <string>
#include <set>

using namespace std;

vector<string_view> SplitIntoWords(string_view str);

template <typename StringContainer>
set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string, std::less<>> non_empty_strings;
    for (const auto str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(string(str));
        }
    }
    return non_empty_strings;
}
