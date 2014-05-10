#include <kazbase/exceptions.h>
#include <kazbase/regex.h>

#include "python.h"

namespace delimit {
namespace parser {

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

    int current_line = 0;
    std::vector<unicode> lines = data.split("\n");

    int tabsize = 0;

    regex::Regex endprog;

    std::vector<Token> result;

    while(true) {
        unicode line;
        if(current_line < lines.size()) {
            line = lines.at(current_line);
        }

        lnum += 1; //Increment the line counter
        uint32_t pos = 0, max = line.length(); //Store the line boundaries
        if(!contstr.empty()) {
            if(line.empty()) {
                throw ValueError("EOF in multiline string");
            }

            regex::Match endmatch = regex::match(endprog, line);
            if(endmatch.matched) {
                //Continue
            }
            //Continue
        } else if(parentlev == 0 && !continued) {
            if(line.empty()) {
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
            }

            if(pos == max) {
                break;
            }

            if(std::string("#\r\n").find(line[pos]) != std::string::npos) {
                handle_newline(line, lnum, pos, result);
                continue;
            }

            if(column > indents.back()) {
                indents.push_back(column);
                result.push_back(
                    Token({
                        TokenType::INDENT,
                        line.slice(nullptr, pos),
                        std::make_pair(lnum, 0),
                        std::make_pair(lnum, line.length())
                    })
                );
            }

            while(column < indents.back()) {
                if(std::find(indents.begin(), indents.end(), column) == indents.end()) {
                    throw ValueError("Unindent doesn't match outer indent level");
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
            }
        } else {
            if(line.empty()) {
                throw ValueError("EOF in multi-line statement");
            }
            continued = 0;
        }

    }

}

std::vector<ScopePtr> Python::parse(const unicode& data)  {
    auto tokens = tokenize(data);

    //TODO: Build scopes
}

}
}
