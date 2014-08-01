#ifndef PYTHON_H
#define PYTHON_H

#include "../base.h"

namespace delimit {
namespace parser {

enum TokenType {
    ENDMARKER,
    NAME,
    NUMBER,
    STRING,
    NEWLINE,
    INDENT,
    DEDENT,
    LPAR,
    RPAR,
    LSQB,
    RSQB,
    COLON,
    COMMA,
    SEMI,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    VBAR,
    AMPER,
    LESS,
    GREATER,
    EQUAL,
    DOT,
    PERCENT,
    BACKQUOTE,
    LBRACE,
    RBRACE,
    EQEQUAL,
    NOTEQUAL,
    LESSEQUAL,
    GREATEREQUAL,
    TILDE,
    CIRCUMFLEX,
    LEFTSHIFT,
    RIGHTSHIFT,
    DOUBLESTAR,
    PLUSEQUAL,
    MINEQUAL,
    STAREQUAL,
    SLASHEQUAL,
    PERCENTEQUAL,
    AMPEREQUAL,
    VBAREQUAL,
    CIRCUMFLEXEQUAL,
    LEFTSHIFTEQUAL,
    RIGHTSHIFTEQUAL,
    DOUBLESTAREQUAL,
    DOUBLESLASH,
    DOUBLESLASHEQUAL,
    AT,
    OP,
    ERRORTOKEN,
    COMMENT,
    NL,
    N_TOKENS,
    NT_OFFSET
};

struct Token {
    TokenType type;
    unicode str;
    std::pair<int, int> start_pos; //Line, col
    std::pair<int, int> end_pos; //Line, col
};

class Python : public delimit::FileParser {
public:
    const unicode name() const { return "PYTHON"; }
    std::pair<std::vector<ScopePtr>, bool> parse(const unicode& data, const unicode& base_scope);
    unicode base_scope_from_filename(const unicode &filename);
    std::vector<Token> tokenize(const unicode& data);
    bool supports_nested_lookups() const { return true; }
};


}
}

#endif // PYTHON_H
