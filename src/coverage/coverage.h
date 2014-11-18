#ifndef COVERAGE_H
#define COVERAGE_H

#include <vector>
#include <cstdint>
#include <memory>
#include <kazbase/unicode.h>
#include <gtksourceviewmm.h>
#include <kazbase/signals.h>
#include <unordered_map>

namespace delimit {
    class DocumentView;
}

namespace coverage {

class Coverage {
public:
    typedef std::shared_ptr<Coverage> ptr;

    void apply_to_document(delimit::DocumentView* buffer);
    void clear_document(delimit::DocumentView* buffer);

    sig::signal<void ()>& signal_coverage_updated() { return signal_coverage_updated_; }

    virtual ~Coverage() {}
protected:
    sig::signal<void ()> signal_coverage_updated_;

private:
    virtual std::vector<int32_t> find_uncovered_lines(const unicode& filename, const unicode& project_root="") = 0;
};

class PythonCoverage: public Coverage {
public:
    typedef std::shared_ptr<PythonCoverage> ptr;

    std::vector<int32_t> find_uncovered_lines(const unicode &filename, const unicode& project_root="");

    ~PythonCoverage();

private:
    unicode find_coverage_file(const unicode &filename, const unicode &project_root);


    typedef Glib::RefPtr<Gio::FileMonitor> FileMonitor;
    std::unordered_map<unicode, FileMonitor> coverage_monitors_;
};

}

#endif // COVERAGE_H
