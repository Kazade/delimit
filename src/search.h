#ifndef SEARCH_H
#define SEARCH_H

#include <gtkmm.h>

#include "base/unicode.h"

namespace delimit {

class Buffer;

class Search : public Gtk::Box {
public:
    Search() {
        build_widgets();
    }

    void set_buffer(Buffer* buffer);

    void on_entry_changed();
    void on_find_next_clicked();

private:
    int highlight_all(const unicode& string);

    Gtk::Entry find_entry_;
    Gtk::Button find_next_button_;

    void build_widgets();

    Buffer* buffer_ = nullptr;
};

}

#endif // SEARCH_H
