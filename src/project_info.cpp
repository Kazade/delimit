#include "project_info.h"
#include "utils.h"

#include "autocomplete/parsers/python.h"
#include "autocomplete/parsers/plain.h"

namespace delimit {

SymbolArray ProjectInfo::find_symbols(Glib::RefPtr<Gsv::Language>& lang, Glib::RefPtr<Gio::FileInputStream>& stream) {
    char buffer[1024 * 1024];
    gsize read = 0;

    std::string data;

    while(stream->read_all(buffer, 1024 * 1024, read)) {
        data += std::string(buffer, buffer + read);
    }

    if(lang && lang->get_name() == "Python") {
        parser::Python parser;
        auto tokens = parser.tokenize(data);

        for(auto tok: tokens) {

        }

    } else {
        parser::Plain parser;
        auto tokens = parser.tokenize(data);
    }

    return SymbolArray();
}

void ProjectInfo::offline_update(const unicode& filename) {

    auto file = Gio::File::create_for_path(filename);
    if(!file->exists()) {
        return;
    }

    auto lang = guess_language_from_file(file);
    auto stream = file->read();

    // Lock the members for update
    std::lock_guard<std::mutex> lock(mutex_);
    {
        auto it = symbols_by_filename_.find(filename);
        if(it != symbols_by_filename_.end()) {
            for(auto symbol: (*it).second) {
                // Wipe out the symbol from the all symbols array
                symbols_.erase(std::remove(symbols_.begin(), symbols_.end(), symbol), symbols_.end());
            }
            (*it).second.clear(); // Wipe out the symbols by filename
        }

        auto symbols = find_symbols(lang, stream);

        filenames_.insert(filename);
        symbols_by_filename_[filename] = symbols;
        symbols_.insert(symbols.begin(), symbols.end()); // Insert all the found symbols
    }
}

std::vector<unicode> ProjectInfo::file_paths() const {

}

SymbolArray ProjectInfo::symbols() const {

}

void ProjectInfo::add_or_update(const unicode& filename) {

}

void ProjectInfo::remove(const unicode& filename) {

}


}
