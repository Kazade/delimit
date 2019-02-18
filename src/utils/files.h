#pragma once

#include <fstream>
#include <sstream>

#include "unicode.h"

static unicode read_file_contents(const unicode& filename, std::string* encoding_out=nullptr) {
    std::ifstream in(filename.encode().c_str());
    if(!in) {
        throw std::runtime_error((_u("Unable to load file") + filename).encode());
    }

    auto str = [&in]{
      std::ostringstream ss{};
      ss << in.rdbuf();
      return ss.str();
    }();

    std::string enc;

    enc = "utf-8";

    if(str.length() >= 2) {
        auto bom0 = +str[0];
        auto bom1 = +str[1];

        if(bom0 == -1 && bom1 == -2) {
            if(str.length() >= 4) {
                auto bom2 = +str[2];
                auto bom3 = +str[3];

                if(!bom2 && !bom3) {
                    enc = "utf-32";
                } else {
                    enc = "utf-16";
                }
            } else {
                enc = "utf-16";
            }
        }
    }

    if(encoding_out) {
        *encoding_out = enc;
    }

    try {
        unicode ret(str, enc);
        return ret;
    } catch(InvalidEncodingError& e) {
        if(enc == "utf-8") {
            /*
             *  If we tried utf8 and it failed, fallback to trying iso-8859-1
             */
            unicode ret(str, "iso-8859-1");
            ret.encode();
            *encoding_out = "iso-8859-1";
            return ret;
        } else {
            throw;
        }
    }
}


static std::vector<unicode> read_file_lines(const unicode& filename, std::string* encoding_out=nullptr) {
    return read_file_contents(filename, encoding_out).replace("\r\n", "\n").split("\n");
}
