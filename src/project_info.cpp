#include <thread>
#include <iostream>
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
    std::cout << "Running update" << std::endl;
    auto file = Gio::File::create_for_path(filename.encode());
    if(!file->query_exists()) {
        return;
    }

    auto lang = guess_language_from_file(file);
    auto stream = file->read();

    // Lock the members for update
    std::lock_guard<std::mutex> lock(mutex_);
    {
        auto it = symbols_by_filename_.find(filename);
        if(it != symbols_by_filename_.end()) {
            for(auto& symbol: (*it).second) {
                // Wipe out the symbol from the all symbols array
                symbols_.erase(symbol);
            }
            (*it).second.clear(); // Wipe out the symbols by filename
        }

        SymbolArray symbols = find_symbols(filename, lang, stream);

        filenames_.insert(filename);
        symbols_by_filename_[filename] = symbols;
        for(Symbol symbol: symbols) {
            symbols_.insert(symbol);// Insert all the found symbols
        }
    }
}

std::vector<unicode> ProjectInfo::file_paths() const {
    return std::vector<unicode>(filenames_.begin(), filenames_.end());
}

SymbolArray ProjectInfo::symbols() const {
    return std::vector<Symbol>(symbols_.begin(), symbols_.end());
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

void ProjectInfo::add_or_update(const unicode& filename) {
    clear_old_futures();
    futures_.push_back(std::async(std::launch::async, std::bind(&ProjectInfo::offline_update, this, filename)));
}

void ProjectInfo::remove(const unicode& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    filenames_.erase(filename);

    for(auto symbol: symbols_by_filename_[filename]) {
        symbols_.erase(symbol);
    }

    symbols_by_filename_.erase(filename);
}


}
