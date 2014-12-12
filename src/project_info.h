#ifndef PROJECT_INFO_H
#define PROJECT_INFO_H

#include <unordered_map>
#include <kazbase/unicode.h>
#include <set>
#include <gtksourceviewmm.h>
#include <mutex>
#include <future>

namespace delimit {

enum SymbolType {
    NAMESPACE = 0,
    CLASS,
    METHOD,
    FUNCTION,
    VARIABLE,
    UNKNOWN
};

struct Symbol {
    unicode name;
    SymbolType type;
    unicode filename;
    int line_number;

    bool operator==(const Symbol& rhs) const {
        return name == rhs.name && filename == rhs.filename && type == rhs.type && line_number == rhs.line_number;
    }

    const Symbol& operator=(const Symbol& rhs) {
        if(&rhs == this) return *this;

        name = rhs.name;
        type = rhs.type;
        filename = rhs.filename;
        line_number = rhs.line_number;

        return *this;
    }

    bool operator<(const Symbol& rhs) const {
        return name < rhs.name;
    }
};

typedef std::vector<Symbol> SymbolArray;

class ProjectInfo {
public:
    std::vector<unicode> file_paths() const;
    SymbolArray symbols() const;

    void add_or_update(const unicode& filename);
    void remove(const unicode& filename);

private:
    std::mutex mutex_;

    std::unordered_map<unicode, SymbolArray> symbols_by_filename_;
    std::set<unicode> filenames_;
    SymbolArray symbols_;

    void clear_old_futures();
    void offline_update(const unicode& filename);
    SymbolArray find_symbols(const unicode& filename, Glib::RefPtr<Gsv::Language> &lang, Glib::RefPtr<Gio::FileInputStream>& stream);

    std::list<std::future<void>> futures_;
};

}

#endif // PROJECT_INFO_H
