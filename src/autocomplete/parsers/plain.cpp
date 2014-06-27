
#include "plain.h"
#include "../base.h"

namespace delimit {
namespace parser {

std::pair<std::vector<ScopePtr>, bool> Plain::parse(const unicode& data, const unicode& base_scope) {
    std::vector<ScopePtr> scopes;

    auto lines = data.split("\n");
    int end_line = lines.size();
    int end_col = lines.back().length();

    for(unicode word: tokenize(data)) {
        auto new_scope = std::make_shared<Scope>(word);

        new_scope->start_line = 0;
        new_scope->start_col = 0;
        new_scope->end_line = end_line;
        new_scope->end_col = end_col;

        scopes.push_back(new_scope);
    }

    return std::make_pair(scopes, true);
}

unicode Plain::base_scope_from_filename(const unicode &filename) {
    return "";
}

std::vector<unicode> Plain::tokenize(const unicode& data) {
    std::vector<char32_t> buffer;
    std::vector<unicode> result;

    for(auto x: data) {
        if(unicode("().|\n\t /\\!#=+-{}%\"\':,[]").contains(unicode(1, x))) {
            auto word = unicode(buffer.begin(), buffer.end()).strip();
            if(!word.empty()) {
                result.push_back(word);
            }
            buffer.clear();
        } else {
            buffer.push_back(x);
        }
    }
    return result;
}

}

}
