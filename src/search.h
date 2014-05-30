#ifndef SEARCH_H
#define SEARCH_H

#include <gtkmm.h>

#include <kazbase/unicode.h>

namespace delimit {

class Frame;
class Buffer;

class Search : public Gtk::Box {
public:
    Search(Frame* parent);

    void on_entry_changed();
    void on_find_next_clicked();
    void on_replace_clicked();
    void on_replace_all_clicked();

    void on_case_sensitive_changed() {
        on_entry_changed();
    }

    void on_entry_activated();
    bool on_entry_key_press(GdkEventKey* event);

    void show();
    void hide() {
        clear_highlight();
        find_entry_.set_text("");
        Gtk::Box::hide();
    }

    sigc::signal<void> signal_close_requested() { return signal_close_requested_; }

    void _connect_signals();

private:
    std::vector<std::pair<Gtk::TextIter, Gtk::TextIter>> matches_;

    void locate_matches(const unicode& string);
    void clear_matches();
    int32_t find_next_match(const Gtk::TextIter& start);
    int32_t last_selected_match_ = -1;
    void replace_text(Gtk::TextIter &start, Gtk::TextIter& end, Glib::ustring& replacement);


    int highlight_all(const unicode& string);
    int highlight_all(const unicode& string, std::vector<Gtk::TextBuffer::iterator>& start_iters);
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
    void on_buffer_changed(Buffer* buffer);

    Buffer* buffer();

    Frame* frame_;

    sigc::signal<void> signal_close_requested_;
};

}

#endif // SEARCH_H
