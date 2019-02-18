#pragma once

#include <regex>
#include "unicode.h"

unicode regex_escape(const unicode& string);

typedef std::match_results<ustring::const_iterator> usmatch;
typedef std::basic_regex<ustring::value_type> uregex;

std::vector<usmatch> regex_search_all(const unicode& data, const uregex& re);
