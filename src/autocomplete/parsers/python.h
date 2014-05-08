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
    std::vector<ScopePtr> parse(const unicode& data);

private:
    std::vector<Token> tokenize(const unicode& data);
};


}
}

#endif // PYTHON_H
