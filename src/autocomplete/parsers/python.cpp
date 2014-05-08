#include <kazbase/exceptions.h>

#include "python.h"

namespace delimit {
namespace parser {

std::vector<Token> Python::tokenize(const unicode& data) {
    int lnum = 0, parenlev = 0, continued = 0;
    unicode namechars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    unicode numchars = "0123456789";

    unicode contstr = "";
    int needcont = 0;

    unicode contline = "";
    std::vector<int> indents = { 0 };

    int current_line = 0;
    std::vector<unicode> lines = data.split("\n");

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
        }

    }

}

std::vector<ScopePtr> Python::parse(const unicode& data)  {
    auto tokens = tokenize(data);

    //TODO: Build scopes
}

}
}
