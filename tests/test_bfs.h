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

        os::remove_dirs(os::path::join({tmp, "root"}));

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
        assert_equal("/tmp/root/one/one", ret[2]);
        assert_equal("/tmp/root/two", ret[3]);
        assert_equal("/tmp/root/two/one", ret[4]);
        assert_equal("/tmp/root/two/two", ret[5]);
        assert_equal("/tmp/root/two/two/one", ret[6]);
    }

    void test_follows_symlinks() {
        unicode tmp = os::temp_dir();
        auto symlink_dest = os::path::join({tmp, "root", "one", "link"});
        auto symlink_source = os::path::join({tmp, "root", "two"});

        os::make_link(symlink_source, symlink_dest);

        BFS bfs(root);

        auto ret = bfs.run();
        std::sort(ret.begin(), ret.end());

        assert_equal("/tmp/root", ret[0]);
        assert_equal("/tmp/root/one", ret[1]);
        assert_equal("/tmp/root/one/link", ret[2]);
        assert_equal("/tmp/root/one/link/one", ret[3]);
        assert_equal("/tmp/root/one/link/two", ret[4]);
        assert_equal("/tmp/root/one/link/two/one", ret[5]);
        assert_equal("/tmp/root/one/one", ret[6]);
        assert_equal("/tmp/root/two", ret[7]);
        assert_equal("/tmp/root/two/one", ret[8]);
        assert_equal("/tmp/root/two/two", ret[9]);
        assert_equal("/tmp/root/two/two/one", ret[10]);
    }

private:
    unicode root;
};

#endif // TEST_BFS_H

