#include "utils/unicode.h"

#include "rank.h"

namespace delimit {

uint32_t rank(const unicode& str, const unicode& search_text) {
    if(search_text.empty()) {
        return 0;
    }

    int highest_score = 0;
    auto first_c = search_text[0];
    auto strlen = str.length();
    for(uint32_t i = 0; i < strlen; ++i) {
        auto sc = str[i];
        if(sc == first_c) {
            int score = 0;
            uint32_t j = i;
            for(auto c: search_text) {
                int since_last = 10;
                for(; j < str.length(); ++j) {
                    if(str[j] == c) {
                        score += std::max(since_last, 1);
                        ++j;
                        break;
                    }
                    since_last--;
                }
            }

            if(score > highest_score) {
                highest_score = score;
            }
        }
    }

    if(!highest_score) {
        return 0;
    }

    auto length_penalty = abs(str.length() - search_text.length());
    //We scale up the score so that the occurance count and length penalty have little effect
    return (highest_score * 100) - length_penalty;
}

}
