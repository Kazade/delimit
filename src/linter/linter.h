#ifndef LINTER_H
#define LINTER_H

#include <memory>
#include <vector>
#include <kazbase/unicode.h>

namespace delimit {
    class Frame;
    class Buffer;
}

namespace linter {

typedef std::vector<std::pair<int32_t, unicode>> LinterResult;

class Linter {
public:
    typedef std::shared_ptr<Linter> ptr;

    void apply_to_buffer(delimit::Buffer* buffer);
    void clear_buffer(delimit::Buffer* buffer);

private:
    virtual LinterResult find_problematic_lines(const unicode& filename, const unicode& project_root="") = 0;
};

class PythonLinter : public Linter {
public:
    typedef std::shared_ptr<PythonLinter> ptr;

private:
    LinterResult find_problematic_lines(const unicode& filename, const unicode& project_root="");
};

}

#endif // LINTER_H
