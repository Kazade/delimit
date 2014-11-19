#ifndef TEST_PROJECT_INFO_H
#define TEST_PROJECT_INFO_H

#include <kazbase/os.h>
#include <kaztest/kaztest.h>
#include "../src/project_info.h"


class ProjectInfoTests : public TestCase {
public:
    void test_python_parsing() {
        auto test_file = os::path::join(os::path::abs_path(__FILE__), "test_python.py");

        delimit::ProjectInfo info;
        info.add_or_update(test_file);

        assert_equal(1, info.file_paths().size());
        assert_equal(test_file, info.file_paths()[0]);
    }
}

#endif // TEST_PROJECT_INFO_H
