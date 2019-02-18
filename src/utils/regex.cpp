#include "regex.h"

unicode regex_escape(const unicode& string) {
    std::vector<unicode> to_replace = { "\\", "{", "}", "^", "$", "|", "(", ")", "[", "]", "*", "+", "?", "." };

    //Could be faster!
    unicode result = string;
    for(auto u: to_replace) {
        result = result.replace(u, _u("\\") + u);
    }

    return result;
}

std::vector<usmatch> regex_search_all(const unicode& data, const uregex& re) {
    std::vector<usmatch> ret;

    usmatch sm;
    while(std::regex_search(data.begin(), data.end(), sm, re)) {
        ret.push_back(sm);
    }

    return ret;
}
