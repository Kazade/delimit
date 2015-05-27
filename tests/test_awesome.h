#ifndef TEST_AWESOME_H
#define TEST_AWESOME_H

#include <kaztest/kaztest.h>
#include "src/rank.h"

class AwesomeBarTest : public TestCase {
public:
    unicode find_highest_rank(const std::vector<unicode>& haystack, const unicode& needle) {
        auto highest = 0;
        unicode best;
        for(auto str: haystack) {
            auto res = delimit::rank(str, needle);
            if(res > highest) {
                best = str;
                highest = res;
            }
        }
        return best;
    }

    void test_basic_ranking() {
        std::vector<unicode> haystack = {
            "google/appengine/api/datastore.py",
            "google/appengine/ext/db.py",
            "delimit/models.py",
            "some/path/with/models/in/file.py",
            "some/path/with/google.py"
        };

        assert_equal(haystack[0], find_highest_rank(haystack, "datastore"));
        assert_equal(haystack[2], find_highest_rank(haystack, "models"));
        assert_equal(haystack[4], find_highest_rank(haystack, "google"));
    }
};

#endif // TEST_AWESOME_H

