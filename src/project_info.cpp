#include <thread>
#include <iostream>

#include <kazbase/os.h>
#include <kazbase/logging.h>

#include "project_info.h"
#include "utils.h"

#include "autocomplete/parsers/python.h"
#include "autocomplete/parsers/plain.h"

namespace delimit {

SymbolArray ProjectInfo::find_symbols(const unicode& filename, Glib::RefPtr<Gsv::Language>& lang, Glib::RefPtr<Gio::FileInputStream>& stream) {
    char buffer[1024 * 1024];
    gsize read = 0;

    std::string data;

    while(!stream->read_all(buffer, 1024 * 1024, read)) {
        data += std::string(buffer, buffer + read);
    }
    data += std::string(buffer, buffer + read);

    SymbolArray result;

    if(lang && lang->get_name() == "Python") {
        parser::Python parser;
        auto tokens = parser.tokenize(data);

        bool next_token_is_class = false;
        bool next_token_is_function = false;

        int line_number = 0;

        parser::Token last_name_token;

        //FIXME: Handle methods and function arguments
        for(auto tok: tokens) {
            if(tok.type == parser::TokenType::NL) {
                line_number++;
            }

            if(next_token_is_class && tok.type == parser::TokenType::NAME) {
                next_token_is_class = false;

                Symbol new_symbol;
                new_symbol.filename = filename;
                new_symbol.name = tok.str;
                new_symbol.line_number = line_number;
                new_symbol.type = SymbolType::CLASS;

                result.push_back(new_symbol);
            } else if(next_token_is_function && tok.type == parser::TokenType::NAME) {
                next_token_is_function = false;

                Symbol new_symbol;
                new_symbol.filename = filename;
                new_symbol.name = tok.str;
                new_symbol.line_number = line_number;
                new_symbol.type = SymbolType::FUNCTION;

                result.push_back(new_symbol);
            }

            if(tok.str == "class" && tok.type == parser::TokenType::NAME) {
                next_token_is_class = true;
            } else if(tok.str == "def" && tok.type == parser::TokenType::NAME) {
                next_token_is_function = true;
            } else if(tok.str == "=" && tok.type == parser::TokenType::OP) {
                Symbol new_symbol;
                new_symbol.filename = filename;
                new_symbol.name = last_name_token.str;
                new_symbol.line_number = line_number;
                new_symbol.type = SymbolType::VARIABLE;

                result.push_back(new_symbol);
            }

            if(tok.type == parser::TokenType::NAME) {
                last_name_token = tok;
            }
        }

    } else {
        parser::Plain parser;
        auto tokens = parser.tokenize(data);
    }

    return result;
}

void ProjectInfo::offline_update(const unicode& filename) {
    auto file = Gio::File::create_for_path(filename.encode());
    if(!file->query_exists()) {
        return;
    }

    auto lang = guess_language_from_file(file);
    auto stream = file->read();

    remove(filename);

    // Lock the members for update
    std::lock_guard<std::mutex> lock(mutex_);
    {        
        try {
            SymbolArray symbols = find_symbols(filename, lang, stream);
            filenames_.insert(filename);
            symbols_by_filename_[filename] = symbols;

            // Insert all the found symbols
            symbols_.insert(symbols_.end(), symbols.begin(), symbols.end());
        } catch (std::exception& e) {
            L_ERROR(_u("An error occurred while indexing: {0}").format(filename));
            return;
        }
    }
}

std::vector<unicode> ProjectInfo::file_paths() const {
    return std::vector<unicode>(filenames_.begin(), filenames_.end());
}

SymbolArray ProjectInfo::symbols() const {
    return symbols_;
}

void ProjectInfo::clear_old_futures() {
    for(auto it = futures_.begin(); it != futures_.end();) {
        if((*it).wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            it = futures_.erase(it);
        } else {
            ++it;
        }
    }
}

void ProjectInfo::add_or_update(const unicode& filename, bool offline) {
    clear_old_futures();

    if(offline) {
        futures_.push_back(std::async(std::launch::async, std::bind(&ProjectInfo::offline_update, this, filename)));
    } else {
        offline_update(filename);
    }
}

void ProjectInfo::remove(const unicode& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    filenames_.erase(filename);

    for(auto symbol: symbols_by_filename_[filename]) {
        symbols_.erase(std::remove(symbols_.begin(), symbols_.end(), symbol), symbols_.end());
    }

    symbols_by_filename_.erase(filename);
}

void ProjectInfo::recursive_populate(const unicode& directory)  {
    const int CYCLES_UNTIL_REFRESH = 15;
    int cycles_until_gtk_update = CYCLES_UNTIL_REFRESH;
    for(auto thing: os::path::list_dir(directory)) {
        //FIXME: Should use gitignore
        if(thing.starts_with(".") || thing.ends_with(".pyc")) continue;

        auto full_path = os::path::join(directory, thing);
        if(os::path::is_dir(full_path)) {
            std::thread(std::bind(&ProjectInfo::recursive_populate, this, full_path)).detach();
        } else {
            add_or_update(full_path, false);

            cycles_until_gtk_update--;
            if(!cycles_until_gtk_update) {
                cycles_until_gtk_update = CYCLES_UNTIL_REFRESH;

                //Keep the window responsive
                while(Gtk::Main::events_pending()) {
                    Gtk::Main::iteration();
                }
            }
        }
    }
}

}
