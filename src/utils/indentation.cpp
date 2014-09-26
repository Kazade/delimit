#include "indentation.h"

namespace delimit {

std::pair<IndentType, int> detect_indentation(const unicode& text) {
    if(text.empty()) {
        return std::make_pair(INDENT_SPACES, 4);
    }

    std::vector<unicode> lines = text.split("\n");

    for(auto line: lines) {
        if(line.empty()) {
            continue;
        }

        std::string str = line.encode();
        auto iter = str.find_first_not_of(" \t\r\n"); //Find the first non-whitespace character

        unicode whitespace(std::string(str.begin(), str.begin() + iter)); //Convert back to unicode which is easier to work with

        if(whitespace.empty()) {
            continue; //Move onto the next line
        }

        if(whitespace.starts_with("\t")) {
            return std::make_pair(INDENT_TABS, whitespace.length()); //Guess tab indentation
        } else {
            uint32_t i = 0;

            //Count the spaces
            while(i < whitespace.length() && whitespace[i] == ' ') {
                ++i;
            }

            //Return indent as spaces, then the number
            return std::make_pair(INDENT_SPACES, i);
        }
    }

    return std::make_pair(INDENT_SPACES, 4);
}

}
