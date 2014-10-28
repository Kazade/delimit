#include <kazbase/unicode.h>
#include <iostream>
#include "awesome_bar.h"
#include "window.h"
#include "buffer.h"

namespace delimit {

template<class T>
unsigned int levenshtein_distance(const T &s1, const T & s2) {
    const size_t len1 = s1.length(), len2 = s2.length();
    std::vector<unsigned int> col(len2+1), prevCol(len2+1);

    for (unsigned int i = 0; i < prevCol.size(); i++)
        prevCol[i] = i;
    for (unsigned int i = 0; i < len1; i++) {
        col[0] = i+1;
        for (unsigned int j = 0; j < len2; j++)
            col[j+1] = std::min( std::min(prevCol[1 + j] + 1, col[j] + 1),
                                prevCol[j] + (s1[i]==s2[j] ? 0 : 1) );
        col.swap(prevCol);
    }
    return prevCol[len2];
}

uint32_t rank(const unicode& str, const unicode& search_text) {
    int score = 0;
    int i = 0;
    for(auto c: search_text) {
        int since_last = 5;
        bool this_c_found = false;
        for(; i < (int) str.length(); ++i) {
            if(str[i] == c) {
                this_c_found = true;
                score += std::max(since_last, 1);
                ++i;
                break;
            }
            since_last--;
        }

        if(!this_c_found) {
            return 0;
        }
    }

    auto length_penalty = abs(str.length() - search_text.length());

    //We scale up the score so that the occurance count and length penalty have little effect
    return (score * 100) + str.count(search_text) - length_penalty;
}

AwesomeBar::AwesomeBar(Window &parent):
    window_(parent) {

    build_widgets();
}

void recursive_populate(std::vector<unicode>* output, const unicode& directory)  {
    const int CYCLES_UNTIL_REFRESH = 15;
    int cycles_until_gtk_update = CYCLES_UNTIL_REFRESH;
    for(auto thing: os::path::list_dir(directory)) {
        //FIXME: Should use gitignore
        if(thing.starts_with(".") || thing.ends_with(".pyc")) continue;

        auto full_path = os::path::join(directory, thing);
        if(os::path::is_dir(full_path)) {
            Glib::signal_idle().connect_once(sigc::bind(&recursive_populate, output, full_path));
        } else {
            output->push_back(full_path);

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

void AwesomeBar::repopulate_files() {
    project_files_.clear();

    if(!window_.project_path().empty()) {
        //If this is a project window, then crawl to get the file list
        recursive_populate(&project_files_, window_.project_path());

    } else {
        //FIXME: We need to just look at open files, and keep this updated
    }
}

void AwesomeBar::build_widgets() { 
    set_margin_left(20);
    set_margin_right(20);
    set_margin_top(20);
    set_margin_bottom(20);
    set_no_show_all();
    set_valign(Gtk::ALIGN_START);
    set_halign(Gtk::ALIGN_CENTER);

    entry_.set_width_chars(32);
    pack_start(entry_, false, false);

    entry_.signal_changed().connect([&]() {
        populate(entry_.get_text().c_str());
    });

    entry_.signal_activate().connect(sigc::mem_fun(this, &AwesomeBar::execute));

    Pango::FontDescription desc("sans-serif 26");
    entry_.override_font(desc);

    pack_start(list_revealer_, false, true, 0);

    list_window_.set_size_request(-1, 500);

    list_window_.add(list_);
    list_revealer_.add(list_window_);

    list_.set_hexpand(false);

    list_.signal_row_activated().connect([this](Gtk::ListBoxRow* row){
        auto file = displayed_files_.at(row->get_index());
        window_.open_document(file);
        hide();
    });
}

void AwesomeBar::execute() {
    hide();
    entry_.set_text("");
}

std::vector<unicode> AwesomeBar::filter_project_files(const unicode& search_text) {
    const int DISPLAY_LIMIT = 30;

    struct Entry {
        unicode path;
        unicode rel;
        uint32_t rank;
    };

    static std::unordered_map<unicode, std::vector<Entry> > CACHE;

    std::vector<unicode> to_add;
    std::vector<Entry> haystack;

    unicode longest_cached;

    //Work out what the longest entry in the cache was
    for(auto p: CACHE) {
        if(p.first.length() > longest_cached.length()) {
            longest_cached = p.first;
        }
    }

    //Wipe out the cache if the text no longer matches
    if(longest_cached.empty() || search_text.length() < longest_cached.length() || !(search_text.starts_with(longest_cached))) {
        std::cout << "Clearing the cache" << std::endl;
        CACHE.clear();

        //Reset the CACHE
        haystack.reserve(project_files_.size());
        for(auto file: project_files_) {
            auto rel = file.slice(window_.project_path().length() + 1, nullptr);
            auto rk = rank(rel.lower(), search_text.lower());
            if(rk) {
                haystack.push_back(Entry{file, rel, rk});
            }
        }

        CACHE[search_text] = haystack;
    } else {
        std::cout << "Hitting the cache" << std::endl;
        //We can use the cache
        auto old_haystack = CACHE[longest_cached];

        haystack.reserve(old_haystack.size());
        //Update the rank with the new text
        for(auto& entry: old_haystack) {
            entry.rank = rank(entry.rel.lower(), search_text.lower());
            if(entry.rank) {
                haystack.push_back(entry);
            }
        }

        CACHE[search_text] = haystack;
    }

    int to_display = std::min(DISPLAY_LIMIT, (int) haystack.size());
    if(to_display) {
        std::partial_sort(haystack.begin(), haystack.begin() + to_display, haystack.end(), [](const Entry& lhs, const Entry& rhs) -> bool { return lhs.rank > rhs.rank; });

        for(int i = 0; i < to_display; ++i) {
            to_add.push_back(haystack[i].path);
        }
    }

    return to_add;
}

void AwesomeBar::populate_results(const std::vector<unicode>& to_add) {
    Pango::FontDescription desc("sans-serif 12");

    displayed_files_.clear();
    for(auto file: to_add) {
        auto to_display = file.slice(window_.project_path().length() + 1, nullptr);
        Gtk::Label* label = Gtk::manage(new Gtk::Label(to_display.encode()));
        label->set_margin_top(10);
        label->set_margin_bottom(10);
        label->set_margin_left(10);
        label->set_margin_right(10);
        label->set_alignment(0, 0.5);
        label->set_hexpand(false);
        label->set_line_wrap_mode(Pango::WRAP_CHAR);
        label->set_line_wrap(true);
        label->override_font(desc);
        list_.append(*label);
        displayed_files_.push_back(file);
    }

    list_.show_all();
}

void AwesomeBar::populate(const unicode &text) {
    //Clear the listing
    for(auto child: list_.get_children()) {
        list_.remove(*child);
    }
    list_revealer_.set_reveal_child(false);

    if(text.starts_with(":") && text.length() > 1) {
        try {
            unicode number_text = text.lstrip(":");
            int line_number = number_text.to_int();

            auto buffer = window_.current_buffer()->buffer();
            auto iter = buffer->get_iter_at_line(line_number);
            window_.current_buffer()->view().scroll_to(iter, 0, 0.5, 0.5);
        } catch(std::exception& e) {
            return;
        }
    } else if(!text.empty()) {        

        auto to_add = filter_project_files(text);
        populate_results(to_add);

    }

    if(!list_.get_children().empty()) {
        list_revealer_.set_reveal_child(true);
    }
}

}
