#ifndef INDENTATION_H
#define INDENTATION_H

#include <kazbase/unicode.h>

namespace delimit {

enum IndentType {
    INDENT_TABS,
    INDENT_SPACES
};

std::pair<IndentType, int> detect_indentation(const unicode& text);

}

#endif // INDENTATION_H
