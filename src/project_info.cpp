#include <thread>
#include <iostream>
#include <queue>

#include <kazbase/os.h>
#include <kazbase/logging.h>

#include "project_info.h"
#include "utils.h"
#include "utils/bfs.h"
#include "utils/sigc_lambda.h"
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
            //filenames_.insert(filename);
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
    //filenames_.erase(filename);

    for(auto symbol: symbols_by_filename_[filename]) {
        symbols_.erase(std::remove(symbols_.begin(), symbols_.end(), symbol), symbols_.end());
    }

    symbols_by_filename_.erase(filename);
}

std::vector<unicode> ProjectInfo::filenames_including(const std::vector<char32_t>& characters) {
    std::unordered_set<unicode*> results;
    std::unordered_set<unicode*> new_results;

    std::unordered_set<char32_t> distinct_chars(characters.begin(), characters.end());

    results.reserve(10000);
    for(char32_t c: distinct_chars) {
        auto& tmp = filenames_including_character_[c];
        if(results.empty()) {
            results.insert(tmp.begin(), tmp.end());
            continue;
        }

        new_results.clear();
        new_results.reserve(results.size());
        for(auto ptr: tmp) {
            if(results.count(ptr)) {
                new_results.insert(ptr);
            }
        }
        results = new_results;
    }

    std::vector<unicode> ret;
    ret.reserve(results.size());

    for(auto ptr: results) {
        ret.push_back(*ptr);
    }

    return ret;
}

void ProjectInfo::update_files(const std::vector<unicode> &new_files) {
    filenames_.assign(new_files.begin(), new_files.end());
    filenames_including_character_.clear();

    for(auto& file: filenames_) {
        std::unordered_set<char32_t> distinct(file.begin(), file.end());
        for(char32_t c: distinct) {
            filenames_including_character_[c].insert(&file);
        }
    }
}

void ProjectInfo::recursive_populate(const unicode& directory)  {
    auto all_files = std::make_shared<BFS>(directory);

    /*
     *  When each level of the tree has been processed, update the files list in the idle
     *  of the main thread. This way, we can keep scanning in the background without blocking
     *  the main thread.
     */
    all_files->signal_level_complete().connect([=](const std::vector<unicode>& result, int level) {
        Glib::signal_idle().connect_once([=]() {
            if((level % 5) == 0) { //Perform an update every 5 levels
                update_files(result);
            }
        });
    });

    struct Wrapper {
        std::future<std::vector<unicode>> wrapped;
    };

    auto wrapper = std::make_shared<Wrapper>();
    wrapper->wrapped = std::async(std::launch::async, std::bind(&BFS::run, all_files));

    Glib::signal_idle().connect([=]() -> bool {
        if(wrapper->wrapped.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            auto result = wrapper->wrapped.get();
            update_files(result);
            return false;
        }

        return true;
    });
}

}
