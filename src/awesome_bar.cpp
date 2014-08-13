#include <kazbase/unicode.h>
#include <iostream>
#include "awesome_bar.h"
#include "window.h"
#include "buffer.h"

namespace delimit {

AwesomeBar::AwesomeBar(Window &parent):
    window_(parent) {

    build_widgets();
}

void recursive_populate(std::vector<unicode>* output, const unicode& directory)  {

    for(auto thing: os::path::list_dir(directory)) {
        //FIXME: Should use gitignore
        if(thing.starts_with(".") || thing.ends_with(".pyc")) continue;

        auto full_path = os::path::join(directory, thing);
        if(os::path::is_dir(full_path)) {
            Glib::signal_idle().connect_once(sigc::bind(&recursive_populate, output, full_path));
        } else {
            output->push_back(full_path);
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

void AwesomeBar::populate(const unicode &text) {
    Pango::FontDescription desc("sans-serif 12");

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
        int counter = 0;
        std::vector<unicode> to_add;

        for(auto file: project_files_) {
            if(file.starts_with(text) || file.contains(text)) {

                to_add.push_back(file);

                if(++counter == 30) {
                    break;
                }
            }            
        }

        std::sort(to_add.begin(), to_add.end());

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

    if(!list_.get_children().empty()) {
        list_revealer_.set_reveal_child(true);
    }
}

}
