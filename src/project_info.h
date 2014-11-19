#ifndef PROJECT_INFO_H
#define PROJECT_INFO_H

#include <unordered_map>
#include <kazbase/unicode.h>
#include <set>
#include <gtksourceviewmm.h>

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
};

typedef std::vector<Symbol> SymbolArray;

class ProjectInfo {
public:
    std::vector<unicode> file_paths() const;
    SymbolArray symbols() const;

    void add_or_update(const unicode& filename);
    void remove(const unicode& filename);

private:
    std::unordered_map<unicode, SymbolArray> symbols_by_filename_;
    std::set<unicode> filenames_;
    std::set<Symbol> symbols_;

    void offline_update(const unicode& filename);
    SymbolArray find_symbols(Glib::RefPtr<Gsv::Language> &lang, Glib::RefPtr<Gio::FileInputStream>& stream);
};

}

#endif // PROJECT_INFO_H
