#ifndef OPEN_FILES_LIST_H
#define OPEN_FILES_LIST_H

#include <gtkmm.h>
#include <unordered_map>

#include "../document_view.h"

namespace delimit {
    class Window;
}

namespace _Gtk {

struct OpenFilesEntry {
    unicode name;
    delimit::DocumentView::ptr buffer;
};

class OpenFilesList {
public:
    OpenFilesList(delimit::Window& parent, Gtk::ListBox* list_box);

    void clear();
    void add_entry(const OpenFilesEntry& entry);
    void remove_entry(const OpenFilesEntry& entry);

    OpenFilesEntry find_entry(delimit::DocumentView::ptr document);

    sigc::signal<void, delimit::DocumentView::ptr>& signal_row_activated()  { return signal_row_activated_; }
    sigc::signal<void, delimit::DocumentView::ptr>& signal_row_close_clicked() { return signal_row_close_clicked_; }
private:
    delimit::Window& parent_;
    Gtk::ListBox* list_box_;
    std::vector<OpenFilesEntry> entries_;
    std::unordered_map<Gtk::ListBoxRow*, uint32_t> row_to_entry_index_;

    sigc::signal<void, delimit::DocumentView::ptr> signal_row_activated_;
    sigc::signal<void, delimit::DocumentView::ptr> signal_row_close_clicked_;
};

}

#endif // OPEN_FILES_LIST_H
