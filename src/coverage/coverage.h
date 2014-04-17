#ifndef COVERAGE_H
#define COVERAGE_H

#include <vector>
#include <cstdint>
#include <memory>
#include <kazbase/unicode.h>
#include <gtksourceviewmm.h>

namespace delimit {
    class Frame;
    class Buffer;
}

namespace coverage {

class Coverage {
public:
    typedef std::shared_ptr<Coverage> ptr;

    void apply_to_buffer(delimit::Buffer* buffer);
    void clear_buffer(delimit::Buffer* buffer);

private:
    virtual std::vector<int32_t> find_uncovered_lines(const unicode& filename, const unicode& project_root="") = 0;

    std::vector<Glib::RefPtr<Gsv::Mark>> marks_;
};

class PythonCoverage: public Coverage {
public:
    typedef std::shared_ptr<PythonCoverage> ptr;

    std::vector<int32_t> find_uncovered_lines(const unicode &filename, const unicode& project_root="");
};

}

#endif // COVERAGE_H
