#include <kazbase/exceptions.h>
#include <kazbase/regex.h>
#include <kazbase/list_utils.h>

#include "python.h"

namespace delimit {
namespace parser {

std::string group(const std::vector<std::string>& choices) {
    return _u("({0})").format(_u("|").join(choices)).encode();
}

std::string any(const std::vector<std::string>& choices) {
    return group(choices) + "*";
}

std::string maybe(const std::vector<std::string>& choices) {
    return group(choices) + "?";
}

const std::string Whitespace = R"([ \f\t]*)";
const std::string Comment = R"(#[^\r\n]*)";
const std::string Ignore = Whitespace + any({R"(\\\r?\n)" + Whitespace}) + maybe({Comment});
const std::string Name = R"([a-zA-Z_]\w*)";

const std::string Hexnumber = R"(0[xX][\da-fA-F]+[lL]?)";
const std::string Octnumber = R"((0[oO][0-7]+)|(0[0-7]*)[lL]?)";
const std::string Binnumber = R"(0[bB][01]+[lL]?)";
const std::string Decnumber = R"([1-9]\d*[lL]?)";
const std::string Intnumber = group({Hexnumber, Binnumber, Octnumber, Decnumber});
const std::string Exponent = R"([eE][-+]?\d+)";
const std::string Pointfloat = group({R"(\d+\.\d*)", R"(\.\d+)"}) + maybe({Exponent});
const std::string Expfloat = R"(\d+)" + Exponent;
const std::string Floatnumber = group({Pointfloat, Expfloat});
const std::string Imagnumber = group({R"(\d+[jJ])", Floatnumber + R"([jJ])"});
const std::string Number = group({Imagnumber, Floatnumber, Intnumber});

//Tail end of ' string.
const std::string Single = R"([^'\\]*(?:\\.[^'\\]*)*')";
//Tail end of " string.
const std::string Double = R"([^"\\]*(?:\\.[^"\\]*)*")";
//Tail end of ''' string.
const std::string Single3 = R"([^'\\]*(?:(?:\\.|'(?!''))[^'\\]*)*''')";
//Tail end of """ string.
const std::string Double3 = R"([^"\\]*(?:(?:\\.|"(?!""))[^"\\]*)*""")";
const std::string Triple = group({R"([uUbB]?[rR]?''')", R"([uUbB]?[rR]?""")"});
//Single-line ' or " string.
const std::string String = group({R"([uUbB]?[rR]?'[^\n'\\]*(?:\\.[^\n'\\]*)*')",
               R"([uUbB]?[rR]?"[^\n"\\]*(?:\\.[^\n"\\]*)*")"});

/*Because of leftmost-then-longest match semantics, be sure to put the
longest operators first (e.g., if = came before ==, == would get
recognized as two instances of =).*/
const std::string Operator = group({
    R"(\*\*=?)", R"(>>=?)", R"(<<=?)", R"(<>)", R"(!=)",
    R"(//=?)", R"([+\-*/%&|^=<>]=?)", R"(~)"
});

const std::string Bracket = R"([][(){}])";
const std::string Special = group({R"(\r?\n)", R"([:;.,`@])"});
const std::string Funny = group({Operator, Bracket, Special});

const std::string PlainToken = group({Number, Funny, String, Name});
const std::string TokenRE = Ignore + PlainToken;

// First (or only) line of ' or " string.
const std::string ContStr = group({
    R"([uUbB]?[rR]?'[^\n'\\]*(?:\\.[^\n'\\]*)*)" +
    group({"'", R"(\\\r?\n)"}),
    R"([uUbB]?[rR]?"[^\n"\\]*(?:\\.[^\n"\\]*)*)" +
    group({'"', R"(\\\r?\n)"})
});
const std::string PseudoExtras = group({R"(\\\r?\n|\Z)", Comment, Triple});
const std::string PseudoToken = Whitespace + group({PseudoExtras, Number, Funny, ContStr, Name});

/*int print() {
    std::cout << PseudoToken << std::endl;
    return 0;
}
int i = print();*/

const regex::Regex tokenprog = regex::compile(TokenRE);
const regex::Regex pseudoprog = regex::compile(PseudoToken);
const regex::Regex single3prog = regex::compile(Single3);
const regex::Regex double3prog = regex::compile(Double3);

const std::map<std::string, regex::Regex> endprogs = {
    {"'", regex::compile(Single)},
    {"\"", regex::compile(Double)},
    {"'''", single3prog},
    {"\"\"\"", double3prog},
    {"r'''", single3prog},
    {"r\"\"\"", double3prog},
    {"u'''", single3prog},
    {"u\"\"\"", double3prog},
    {"ur'''", single3prog},
    {"ur\"\"\"", double3prog},
    {"R'''", single3prog},
    {"R\"\"\"", double3prog},
    {"U'''", single3prog},
    {"U\"\"\"", double3prog},
    {"uR'''", single3prog},
    {"uR\"\"\"", double3prog},
    {"Ur'''", single3prog},
    {"Ur\"\"\"", double3prog},
    {"UR'''", single3prog},
    {"UR\"\"\"", double3prog},
    {"b'''", single3prog},
    {"b\"\"\"", double3prog},
    {"br'''", single3prog},
    {"br\"\"\"", double3prog},
    {"B'''", single3prog},
    {"B\"\"\"", double3prog},
    {"bR'''", single3prog},
    {"bR\"\"\"", double3prog},
    {"Br'''", single3prog},
    {"Br\"\"\"", double3prog},
    {"BR'''", single3prog},
    {"BR\"\"\"", double3prog}
};

const std::set<std::string> triple_quoted = {
    "'''", "\"\"\"",
  "r'''", "r\"\"\"", "R'''", "R\"\"\"",
  "u'''", "u\"\"\"", "U'''", "U\"\"\"",
  "ur'''", "ur\"\"\"", "Ur'''", "Ur\"\"\"",
  "uR'''", "uR\"\"\"", "UR'''", "UR\"\"\"",
  "b'''", "b\"\"\"", "B'''", "B\"\"\"",
  "br'''", "br\"\"\"", "Br'''", "Br\"\"\"",
  "bR'''", "bR\"\"\"", "BR'''", "BR\"\"\""
};

const std::set<std::string> single_quoted = {
    "'", "\"",
    "r'", "r\"", "R'", "R\"",
    "u'", "u\"", "U'", "U\"",
    "ur'", "ur\"", "Ur'", "Ur\"",
    "uR'", "uR\"", "UR'", "UR\"",
    "b'", "b\"", "B'", "B\"",
    "br'", "br\"", "Br'", "Br\"",
    "bR'", "bR\"", "BR'", "BR\""
};


const int tabsize = 8;

class TokenizationError : public ValueError {
public:
    TokenizationError(const unicode& what):
        ValueError(what.encode()) {}
};

void handle_newline(const unicode& line, int lnum, uint32_t pos, std::vector<Token>& result) {
    if(line[pos] == '#') {
        auto comment_token = line.slice(pos, nullptr).rstrip("\r\n");
        auto nl_pos = pos + comment_token.length();
        result.push_back(
            Token({
                TokenType::COMMENT,
                comment_token,
                std::make_pair(lnum, pos),
                std::make_pair(lnum, pos + comment_token.length())
            })
        );

        result.push_back(
            Token({
                TokenType::NL,
                line.slice(nl_pos, nullptr),
                std::make_pair(lnum, nl_pos),
                std::make_pair(lnum, line.length())
            })
        );
    } else {
        auto res = line[pos] == '#';

        result.push_back(
            Token({
                (res) ? TokenType::COMMENT : TokenType::NL,
                line.slice(pos, nullptr),
                std::make_pair(lnum, (int)pos),
                std::make_pair(lnum, (int)line.length())
            })
        );
    }
}

std::vector<Token> Python::tokenize(const unicode& data) {
    int lnum = 0, parentlev = 0, continued = 0;
    unicode namechars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    unicode numchars = "0123456789";

    unicode contstr = "";
    int needcont = 0;

    unicode contline = "";
    std::vector<int> indents = { 0 };

    std::vector<unicode> lines = data.split("\n");
    for(uint32_t i = 0; i < lines.size(); ++i) {
        lines[i] = lines[i] + "\n";
    }

    int tabsize = 0;

    regex::Regex endprog;

    std::vector<Token> result;
    std::pair<int, int> strstart;
    int start = 0, end = 0;

    while(true) {
        unicode line;
        if(lnum < (int) lines.size()) {
            line = lines.at(lnum);
        }

      //  std::cout << line << std::endl;

        lnum += 1; //Increment the line counter
        int32_t pos = 0, max = line.length(); //Store the line boundaries
        if(!contstr.empty()) {
            if(line.empty()) {
                throw TokenizationError("EOF in multiline string");
            }

            auto endmatch = endprog.match(line);
            if(endmatch) {
                //Continue
                pos = end = endmatch.end(0);
              //  std::cout << "Found STRING token: " << contstr + line.slice(nullptr, end) << std::endl;
                result.push_back(Token({TokenType::STRING, contstr + line.slice(nullptr, end), strstart, std::make_pair(lnum, end)}));
                contstr = "";
                needcont = 0;
                contline = "";
            } else if(needcont && line.slice(-2, nullptr) != "\n" && line.slice(-3, nullptr) != "\r\n") {
                result.push_back(Token({TokenType::ERRORTOKEN, contstr + line, strstart, std::make_pair(lnum, line.length())}));
                contstr = "";
                contline = "";
                continue;
            } else {
                contstr = contstr + line;
                contline = contline + line;
                continue;
            }
        } else if(parentlev == 0 && !continued) {
            if(line.empty()) {
              //  std::cout << "Breaking loop because the line is empty" << std::endl;
                break;
            }
            int column = 0;
            while(pos < max) {
                if(line[pos] == ' ') {
                    column += 1;
                } else if(line[pos] == '\t') {
                    column = (column / tabsize + 1) * tabsize;
                } else if(line[pos] == '\f') {
                    column = 0;
                } else {
                    break;
                }
                pos += 1;
            }

            if(pos == max) {
                break;
            }

            if(_u("#\r\n").contains(_u(1, line[pos]))) {
              //  std::cout << "Handling newline" << std::endl;
                handle_newline(line, lnum, pos, result);
                continue;
            }

            if(column > indents.back()) {
                indents.push_back(column);
              //  std::cout << "INDENT" << std::endl;
                result.push_back(
                    Token({
                        TokenType::INDENT,
                        line.slice(nullptr, pos),
                        std::make_pair(lnum, 0),
                        std::make_pair(lnum, line.length())
                    })
                );
            }

       //     std::cout << column << "====" << indents.back() << std::endl;
            while(column < indents.back()) {
                if(std::find(indents.begin(), indents.end(), column) == indents.end()) {
                    throw TokenizationError("Unindent doesn't match outer indent level");
                }
                indents.pop_back();
                result.push_back(
                    Token({
                        TokenType::DEDENT,
                        "",
                        std::make_pair(lnum, pos),
                        std::make_pair(lnum, pos)
                    })
                );

       //         std::cout << "DEDENT" << std::endl;
            }
        } else {
            if(line.empty()) {
                throw TokenizationError("EOF in multi-line statement");
            }
            continued = 0;
        }

        unicode token;
        std::pair<int, int> spos;
        std::pair<int, int> epos;
        while(pos < max) {
           // std::cout << pos << std::endl;
            auto pseudomatch = pseudoprog.match(line, pos);
            if(pseudomatch) {
                std::pair<int, int> start_end = pseudomatch.span(1);
                start = start_end.first;
                end = start_end.second;
                spos = std::make_pair(lnum, start);
                epos = std::make_pair(lnum, end);
                pos = end;
                if(start == end) {
                    continue;
                }

                token = line.slice(start, end);
                auto initial = _u(1, line[start]);

                if(numchars.contains(initial) || (initial == "." && token != ".")) {
                    //We have a number
                    //std::cout << "Found NUMBER token: " << token << std::endl;
                    result.push_back(Token({TokenType::NUMBER, token, spos, epos}));
                } else if(_u("\r\n").contains(initial)) {
                    if(parentlev > 0) {
                        result.push_back(Token({TokenType::NL, token, spos, epos}));
                    } else {
                        result.push_back(Token({TokenType::NEWLINE, token, spos, epos}));
                    }
                } else if(initial == _u("#")) {
                    assert(!token.ends_with("\n"));
                    result.push_back(Token({TokenType::COMMENT, token, spos, epos}));
                } else if(triple_quoted.count(token.encode())) {
                    auto endprog = endprogs.at(token.encode());
                    auto endmatch = endprog.match(line, pos);
                    if(endmatch) {
                        pos = endmatch.end(0);
                        token = line.slice(start, pos);
                        result.push_back(Token({TokenType::STRING, token, spos, std::make_pair(lnum, pos)}));
                    } else {
                        strstart = std::make_pair(lnum, start);
                        contstr = line.slice(start, nullptr);
                        contline = line;
                        break;
                    }
                } else if(
                    single_quoted.count(token.encode()) ||
                    single_quoted.count(token.slice(nullptr, 2).encode()) ||
                    single_quoted.count(token.slice(nullptr, 3).encode())) {

                    if(token[token.length() - 1] == '\n') {
                        strstart = std::make_pair(lnum, start);

                        bool break_outer = false;
                        for(unicode tok: { initial, _u(1, token[1]), _u(1, token[2])}) {
                            if(endprogs.count(tok.encode())) {
                                auto endprog = endprogs.at(tok.encode());
                                contstr = line.slice(start, nullptr);
                                needcont = 1;
                                contline = line;
                                break_outer = true;
                                break;
                            }
                        }

                        if(break_outer) {
                            break;
                        }
                    } else {
                        result.push_back(Token({TokenType::STRING, token, spos, epos}));
                    }
                } else if(namechars.contains(initial)) {
                   // std::cout << "Found name token: " << token << std::endl;
                    result.push_back(Token({TokenType::NAME, token, spos, epos}));
                } else if(initial == "\\") {
                    continued = 1;
                } else {
                    if(_u("([{").contains(initial)) {
                        parentlev += 1;
                    } else if(_u(")]}").contains(initial)) {
                        parentlev -= 1;
                    }
                   // std::cout << "Found OP token: " << token << std::endl;
                    result.push_back(Token({TokenType::OP, token, spos, epos}));
                }
            } else {
                result.push_back(Token({TokenType::ERRORTOKEN, token, spos, epos}));
                pos += 1;
            }
        }
    }

    for(uint32_t i = 1; i < indents.size(); ++i) {
        //std::cout << "DEDENT" << std::endl;
        result.push_back(Token({TokenType::DEDENT, "", std::make_pair(lnum, 0), std::make_pair(lnum, 0)}));
    }
    result.push_back(Token({TokenType::ENDMARKER, "", std::make_pair(lnum, 0), std::make_pair(lnum, 0)}));
    return result;
}

bool is_python_builtin(const unicode& name) {
    const std::vector<unicode> builtins = {
        "ArithmeticError", "AssertionError", "AttributeError", "BaseException",
        "BufferError", "BytesWarning", "DeprecationWarning", "EOFError",
        "Ellipsis", "EnvironmentError", "Exception", "False", "FloatingPointError",
        "FutureWarning", "GeneratorExit", "IOError", "ImportError", "ImportWarning",
        "IndentationError", "IndexError", "KeyError", "KeyboardInterrupt",
        "LookupError", "MemoryError", "NameError", "None", "NotImplemented",
        "NotImplementedError", "OSError", "OverflowError", "PendingDeprecationWarning",
        "ReferenceError", "RuntimeError", "RuntimeWarning", "StandardError",
        "StopIteration", "SyntaxError", "SyntaxWarning", "SystemError", "SystemExit",
        "TabError", "True", "TypeError", "UnboundLocalError", "UnicodeDecodeError",
        "UnicodeEncodeError", "UnicodeError", "UnicodeTranslateError", "UnicodeWarning",
        "UserWarning", "ValueError", "Warning", "ZeroDivisionError", "_", "__debug__",
        "__doc__", "__import__", "__name__", "__package__", "abs", "all", "any", "apply",
        "basestring", "bin", "bool", "buffer", "bytearray", "bytes", "callable", "chr",
        "classmethod", "cmp", "coerce", "compile", "complex", "copyright", "credits",
        "delattr", "dict", "dir", "divmod", "enumerate", "eval", "execfile", "exit",
        "file", "filter", "float", "format", "frozenset", "getattr", "globals",
        "hasattr", "hash", "help", "hex", "id", "input", "int", "intern", "isinstance",
        "issubclass", "iter", "len", "license", "list", "locals", "long", "map", "max",
        "memoryview", "min", "next", "object", "oct", "open", "ord", "pow", "print",
        "property", "quit", "range", "raw_input", "reduce", "reload", "repr", "reversed",
        "round", "set", "setattr", "slice", "sorted", "staticmethod", "str", "sum", "super",
        "tuple", "type", "unichr", "unicode", "vars", "xrange", "zip"
    };

    return std::find(builtins.begin(), builtins.end(), name) != builtins.end();
}

std::pair<bool, std::vector<Token>> find_tokens_till_next(const unicode& what, const std::vector<Token>& all_tokens, const int start) {
    bool complete = false;
    int lookahead = start;
    std::vector<Token> result;
    while(true) {
        if(uint32_t(lookahead) > all_tokens.size() - 1) {
            break;
        }

        const Token& lookahead_tok = all_tokens.at(lookahead);
        if(lookahead_tok.str == what) {
            complete = true;
            break;
        }

        result.push_back(lookahead_tok);

        lookahead += 1;
    }

    return std::make_pair(complete, result);
}

std::vector<ScopePtr> Python::parse(const unicode& data)  {
    auto tokens = tokenize(data);

    std::vector<ScopePtr> scopes;

    std::shared_ptr<Token> last_token;
    std::shared_ptr<Token> next_token;

    unicode current_path = "";

    std::vector<ScopePtr> open_scopes;

    bool inside_class = false;

    for(uint32_t i = 0; i < tokens.size(); ++i) {
        if(i > 0) {
            last_token = std::make_shared<Token>(tokens[i - 1]);
        }

        if(i < tokens.size() - 1) {
            next_token = std::make_shared<Token>(tokens[i + 1]);
        } else {
            next_token.reset();
        }

        Token& this_token = tokens.at(i);


        if(this_token.type == TokenType::NAME) {
            //If this token is the class keyword, and we have the next token, then
            //process a class
            if(this_token.str == "class" && next_token && next_token->type == TokenType::NAME) {
                unicode new_scope_path = _u(".").join({current_path, next_token->str});

                std::vector<unicode> inherited_paths;
                auto search = find_tokens_till_next(")", tokens, i + 2);

                for(auto lookahead_tok: search.second) {
                    if(lookahead_tok.type == TokenType::NAME) {
                        //FIXME: If an import or other thing further up overrides a global, we should prefer that
                        // e.g. if someone overrides "object"

                        //If this is a Python builtin, then we don't include the current path when
                        //adding to the inherited scopes
                        if(is_python_builtin(lookahead_tok.str)) {
                            inherited_paths.push_back(lookahead_tok.str);
                        } else {
                            inherited_paths.push_back(_u(".").join({current_path, lookahead_tok.str}));
                        }
                    }

                }

                ScopePtr new_scope = std::make_shared<Scope>(new_scope_path, inherited_paths);
                new_scope->start_line = this_token.start_pos.first;
                new_scope->start_col = this_token.start_pos.second;
                scopes.push_back(new_scope);
                open_scopes.push_back(new_scope);

                current_path = new_scope_path;
                inside_class = true;
            } else if(this_token.str == "def" && next_token && next_token->type == TokenType::NAME) {
                //We've found a function or method
                unicode new_scope_path = _u(".").join({current_path, next_token->str});
                std::vector<unicode> inherited_scopes;
                if(inside_class) {
                    inherited_scopes.push_back("instancemethod");
                } else {
                    inherited_scopes.push_back("function");
                }
                ScopePtr new_scope = std::make_shared<Scope>(new_scope_path, inherited_scopes);
                new_scope->start_line = this_token.start_pos.first;
                new_scope->start_col = this_token.end_pos.second; //Start the scope at the end of the method name


                std::vector<ScopePtr> args;

                auto search = find_tokens_till_next(")", tokens, i + 3);

                bool next_token_is_assignment = false;
                bool next_name_is_args = false;
                bool next_name_is_kwargs = false;
                for(auto& lookahead_tok: search.second) {
                    if(lookahead_tok.type == TokenType::NAME) {
                        if(!next_token_is_assignment) {
                            if(next_name_is_args || next_name_is_kwargs) {
                                std::cout << "Need to set the current type (e.g. tuple/dict)" << std::endl;
                            }

                            unicode arg_scope_path = _u(".").join({new_scope_path, lookahead_tok.str});
                            ScopePtr new_arg = std::make_shared<Scope>(arg_scope_path);
                            new_arg->start_line = this_token.start_pos.first;
                            new_arg->start_col = this_token.end_pos.second; //Inherit function scope boundaries
                            args.push_back(new_arg);
                        } else {
                            next_token_is_assignment = false;
                            std::cout << "Default arguments to methods not yet handled" << std::endl;
                        }
                    } else if(lookahead_tok.type == TokenType::OP) {
                        if(lookahead_tok.str == ",") {
                            continue;
                        } else if(lookahead_tok.str == "=") {
                            next_token_is_assignment = true;
                        } else if(lookahead_tok.str == "*") {
                            next_name_is_args = true;
                        } else if(lookahead_tok.str == "**") {
                            next_name_is_kwargs = true;
                        } else {
                            std::cout << "Unexpected token during function definition: " << lookahead_tok.str << std::endl;
                        }
                    } else {
                        std::cout << "Unexpected token type during function definition: " << lookahead_tok.type << " - " << lookahead_tok.str << std::endl;
                    }
                }

                scopes.push_back(new_scope);
                for(auto scope: args) {
                    scopes.push_back(scope);
                }

            } else if(next_token && next_token->type == TokenType::OP) {
                if(next_token->str == "=") {
                    //Let's figure out what's been assigned
                    int lookahead = i + 2;

                    std::vector<unicode> inherited_scopes;
                    if(lookahead < (int) tokens.size()) {
                        Token& tok = tokens.at(lookahead);

                        if(tok.type == OP) {
                            if(tok.str == "[") {
                                inherited_scopes.push_back("list");
                            } else if(tok.str == "{") {
                                //FIXME: Could be a set, we should look for a ":" before a closing }
                                inherited_scopes.push_back("dict");
                            } else if(tok.str.starts_with("'") || tok.str.starts_with("\"")) {
                                inherited_scopes.push_back("str");
                            } else {
                                std::cout << "Unhandled OP: " << tok.str << std::endl;
                            }
                        } else if(tok.type == TokenType::NUMBER) {
                            if(tok.str.contains(".")) {
                                inherited_scopes.push_back("float");
                            } else if(tok.str.ends_with("L")) {
                                inherited_scopes.push_back("long");
                            } else {
                                inherited_scopes.push_back("int");
                            }
                        } else if(tok.type == TokenType::NAME) {
                            if(is_python_builtin(tok.str)) {
                                inherited_scopes.push_back(tok.str);
                            } else {
                                //??? We need to look back up the scope tree from the current scope to
                                // find a matching scope with this name... e.g. if we are doing a = A() we
                                // need to go through the scopes and find the deepest scope where the last part of the path
                                // is 'A'
                                std::cout << "Unhandled assignment: " << tok.type << " - " << tok.str << std::endl;
                            }

                        } else {
                            std::cout << "Unhandled assignment: " << tok.type << " - " << tok.str << std::endl;
                        }


                    } else {
                        //No inherited scopes... perhaps it should inherit something by default?
                    }


                    //Assignment to something \o/
                    unicode new_scope_path = _u(".").join({current_path, this_token.str});
                    ScopePtr new_scope = std::make_shared<Scope>(new_scope_path, inherited_scopes);
                    new_scope->start_line = this_token.start_pos.first;
                    new_scope->start_col = next_token->start_pos.second; //Start after the assignment
                    scopes.push_back(new_scope);
                    open_scopes.push_back(new_scope);
                }
            }
        } else if(this_token.type == TokenType::DEDENT) {
            for(auto scope: open_scopes) {
                scope->end_line = this_token.end_pos.first;
                scope->end_col = this_token.end_pos.second;
            }
            open_scopes.clear();
        }
    }

    return scopes;
}

}
}
