#include <kazbase/unicode.h>
#include <iostream>
#include "utils/sigc_lambda.h"
#include "awesome_bar.h"
#include "window.h"
#include "project_info.h"
#include "rank.h"

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

// Ubuntu Trusty sucks and ships an ancient Gtk, one that apparently bugily
// causes focus out events when you select a row in a list box. So we have this stupid
// flag which controls regrabbing the focus and repositioning the caret so the whole
// thing doesn't go to shit
static bool about_to_focus = false;

void AwesomeBar::build_widgets() { 
    set_margin_left(20);
    set_margin_right(20);
    set_margin_top(20);
    set_margin_bottom(20);
    set_no_show_all();
    set_valign(Gtk::ALIGN_START);
    set_halign(Gtk::ALIGN_CENTER);

    entry_.set_width_chars(50);
    pack_start(entry_, false, false);

    entry_.signal_changed().connect([&]() {
        populate(entry_.get_text().c_str());
    });

    entry_.signal_activate().connect(sigc::mem_fun(this, &AwesomeBar::execute));
    entry_.signal_focus_out_event().connect([=](GdkEventFocus* evt) -> bool {
        if(about_to_focus) {
            about_to_focus = false;
            entry_.grab_focus();
            entry_.set_position(-1);
            return true;
        }
        entry_.hide();
        entry_.set_text("");
        return false;
    });

    entry_.signal_key_press_event().connect([=](GdkEventKey* evt) -> bool {
        if(evt->keyval == GDK_KEY_Down || evt->keyval == GDK_KEY_Up) {
            /*
             * If we key up or down, change the selection in the list but
             * return false so that we don't lose focus on the box
             */

            auto row = list_.get_selected_row();
            if(!row) {
                return false;
            }

            auto idx = row->get_index();

            idx += (evt->keyval == GDK_KEY_Down) ? 1 : -1;

            idx = std::max(idx, 0);
            idx = std::min(idx, (int32_t) displayed_files_.size());

            about_to_focus = true;
            list_.select_row(*list_.get_row_at_index(idx));

            return true;
        }

        return false;
    });

    Pango::FontDescription desc("sans-serif 16");
    entry_.override_font(desc);

    pack_start(list_revealer_, false, true, 0);

    list_window_.set_size_request(-1, 650);

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
    if(filter_task_) {
        // Still waiting for results to come back
        return;
    } else {
        // If we have some results, then activate the first one!
        if(list_.get_children().size()) {
            auto row = list_.get_row_at_index(0);
            auto file = displayed_files_.at(row->get_index());
            window_.open_document(file);
        }
    }
    hide();
    entry_.set_text("");
}

class TaskTerminatedError : public std::exception {};

std::vector<unicode> AwesomeBar::filter_project_files(const unicode& search_text, uint64_t filter_task_id) {
    unicode lower_text = search_text.lower();

    const int DISPLAY_LIMIT = 15;

    std::vector<unicode> haystack;

    if(this->filter_task_id_ != filter_task_id) {
        return std::vector<unicode>();
    }

    haystack = this->window_.info()->filenames_including(std::vector<char32_t>(lower_text.begin(), lower_text.end()));

    int to_display = std::min(DISPLAY_LIMIT, (int) haystack.size());

    int project_path_length = window_.project_path().length() + 1;

    try {
        if(this->filter_task_id_ != filter_task_id) {
            return std::vector<unicode>();
        }

        std::partial_sort(haystack.begin(), haystack.begin() + to_display, haystack.end(), [=](const unicode& lhs, const unicode& rhs) -> bool {
            if(this->filter_task_id_ != filter_task_id) {
                throw TaskTerminatedError();
            }
            auto lhs_rel = lhs.slice(project_path_length, nullptr).lower();
            auto rhs_rel = rhs.slice(project_path_length, nullptr).lower();
            return rank(lhs_rel, lower_text) > rank(rhs_rel, lower_text);
        });
    } catch(TaskTerminatedError&e ) {
        return std::vector<unicode>();
    }

    return std::vector<unicode>(haystack.begin(), haystack.begin() + to_display);
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
        label->set_ellipsize(Pango::ELLIPSIZE_MIDDLE);
        label->set_line_wrap(true);
        label->override_font(desc);
        label->set_can_focus(false);
        list_.append(*label);
        displayed_files_.push_back(file);
    }

    list_.show_all();
    if(!list_.get_children().empty()) {
        list_revealer_.set_reveal_child(true);
        about_to_focus = true;
        list_.select_row(*list_.get_row_at_index(0));

    }
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

            window_.current_buffer()->scroll_to_line(line_number);
        } catch(std::exception& e) {
            return;
        }
    } else if(!text.empty()) {
        filter_task_ = std::make_shared<std::future<std::vector<unicode>>>(
            std::async(std::launch::async, std::bind(&AwesomeBar::filter_project_files, this, text, ++filter_task_id_))
        );

        auto copy = filter_task_;
        Glib::signal_idle().connect([&, copy]() -> bool {
            if(!filter_task_ || filter_task_.get() != copy.get()) {
                //A new task was submitted or the task was cancelled
                return false;
            }

            if(filter_task_->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                populate_results(filter_task_->get());
                filter_task_.reset();
                return false;
            }

            return true;
        });
    }
}

}
