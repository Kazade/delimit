#ifndef SEARCH_H
#define SEARCH_H

#include <gtkmm.h>

#include "base/unicode.h"

namespace delimit {

class Buffer;

class Search : public Gtk::Box {
public:
    Search():
        buffer_(nullptr) {
        build_widgets();
    }

    void set_buffer(Buffer* buffer);

    void on_entry_changed();
    void on_find_next_clicked();
    void on_case_sensitive_changed() {
        on_entry_changed();
    }

    bool on_entry_key_press(GdkEventKey* event);

    void show() {
        Gtk::Box::show();
        find_entry_.grab_focus();

        toggle_replace(false);
    }

    void hide() {
        clear_highlight();
        find_entry_.set_text("");
        Gtk::Box::hide();
    }

    sigc::signal<void> signal_close_requested() { return signal_close_requested_; }

private:
    int highlight_all(const unicode& string);
    void clear_highlight();

    void toggle_replace(bool show) {
        if(show) {
            replace_all_button_.show();
            replace_button_.show();
            replace_entry_.show();
        } else {
            replace_all_button_.hide();
            replace_button_.hide();
            replace_entry_.hide();
        }
    }

    Gdk::RGBA default_entry_colour_;
    Gtk::Entry find_entry_;
    Gtk::Entry replace_entry_;
    Gtk::Button find_next_button_;
    Gtk::Button replace_button_;
    Gtk::Button replace_all_button_;
    Gtk::Button close_button_;

    Gtk::CheckButton case_sensitive_;

    void build_widgets();

    Buffer* buffer_;

    sigc::signal<void> signal_close_requested_;
};

}

#endif // SEARCH_H
