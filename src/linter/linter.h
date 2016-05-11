#ifndef LINTER_H
#define LINTER_H

#include <map>
#include <memory>
#include <vector>
#include <kazbase/unicode.h>

namespace delimit {
    class DocumentView;

    typedef std::map<int32_t, unicode> ErrorList;
}

namespace linter {

class Linter {
public:
    typedef std::shared_ptr<Linter> ptr;

    void apply_to_document(delimit::DocumentView* buffer);
    void clear_document(delimit::DocumentView* buffer);

private:
    virtual delimit::ErrorList find_problematic_lines(const unicode& filename, const unicode& project_root="") = 0;
};

class PythonLinter : public Linter {
public:
    typedef std::shared_ptr<PythonLinter> ptr;

private:
    delimit::ErrorList find_problematic_lines(const unicode& filename, const unicode& project_root="");
};

class JavascriptLinter : public Linter {
public:
    typedef std::shared_ptr<JavascriptLinter> ptr;

private:
    delimit::ErrorList find_problematic_lines(const unicode& filename, const unicode& project_root="");
};

}

#endif // LINTER_H
