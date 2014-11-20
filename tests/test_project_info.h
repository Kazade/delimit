#ifndef TEST_PROJECT_INFO_H
#define TEST_PROJECT_INFO_H

#include <kazbase/os.h>
#include <kaztest/kaztest.h>
#include "../src/project_info.h"


class ProjectInfoTests : public TestCase {
public:
    void set_up() {
        Gsv::init();

        TestCase::set_up();
    }

    void test_python_parsing() {
        auto test_file = os::path::join(os::path::dir_name(os::path::abs_path(__FILE__)), "test_python.py");

        delimit::ProjectInfo info;
        info.add_or_update(test_file);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        assert_equal(1, info.file_paths().size());
        assert_equal(test_file, info.file_paths()[0]);

        auto symbols = info.symbols();

        assert_equal(3, symbols.size());

        assert_equal("A", symbols[0].name);
        assert_equal("main", symbols[0].name);
        assert_equal("a", symbols[0].name);
    }
};

#endif // TEST_PROJECT_INFO_H
