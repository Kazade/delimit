#ifndef TEST_BFS_H
#define TEST_BFS_H

#include <algorithm>
#include <kazbase/os.h>
#include <kaztest/kaztest.h>
#include "../src/utils/bfs.h"

class BFSTest : public TestCase {
public:
    void set_up() {
        TestCase::set_up();

        unicode tmp = os::temp_dir();

        std::vector<unicode> to_create;
        to_create.push_back(os::path::join({tmp, "root"}));
        to_create.push_back(os::path::join({tmp, "root", "one"}));
        to_create.push_back(os::path::join({tmp, "root", "one", "one"}));
        to_create.push_back(os::path::join({tmp, "root", "two"}));
        to_create.push_back(os::path::join({tmp, "root", "two", "one"}));
        to_create.push_back(os::path::join({tmp, "root", "two", "two"}));
        to_create.push_back(os::path::join({tmp, "root", "two", "two", "one"}));

        root = to_create[0];

        for(auto f: to_create) {
            os::make_dirs(f);
        }
    }

    void test_recursive_listing() {
        BFS bfs(root);

        auto ret = bfs.run();
        std::sort(ret.begin(), ret.end());

        assert_equal("/tmp/root", ret[0]);
        assert_equal("/tmp/root/one", ret[1]);
        assert_equal("/tmp/root/two", ret[2]);
        assert_equal("/tmp/root/one/one", ret[3]);
        assert_equal("/tmp/root/two/one", ret[4]);
        assert_equal("/tmp/root/two/two", ret[5]);
        assert_equal("/tmp/root/two/two/one", ret[6]);
    }

private:
    unicode root;
};

#endif // TEST_BFS_H

