#include <kazbase/exceptions.h>
#include "open_files_list.h"

namespace _Gtk {

OpenFilesList::OpenFilesList(delimit::Window& parent, Gtk::ListBox* list_box):
    parent_(parent),
    list_box_(list_box) {

    list_box_->signal_row_activated().connect([&](Gtk::ListBoxRow* row) {
        auto it = row_to_entry_index_.find(row);
        if(it == row_to_entry_index_.end()) {
            throw RuntimeError("Something went horribly wrong with the open files list");
        }

        signal_row_activated_(entries_.at((*it).second).buffer);
    });
}

void OpenFilesList::clear() {
    for(auto widget: list_box_->get_children()) {
        list_box_->remove(*widget);
    }
    entries_.clear();
    row_to_entry_index_.clear();
}

void OpenFilesList::add_entry(const OpenFilesEntry& entry) {
    Gtk::HBox* box = Gtk::manage(new Gtk::HBox());
    Gtk::Label* label = Gtk::manage(new Gtk::Label(entry.name.encode()));
    Gtk::Button* button = Gtk::manage(new Gtk::Button());

    button->set_relief(Gtk::RELIEF_NONE);
    button->set_image_from_icon_name("window-close");

    button->signal_clicked().connect([entry, this]() {
        this->signal_row_close_clicked_(entry.buffer);
    });

    box->pack_start(*label, true, true, 0);
    box->pack_end(*button, false, false, 0);

    list_box_->set_margin_left(0);
    list_box_->add(*box);
    list_box_->show_all();

    Gtk::ListBoxRow* new_row = list_box_->get_row_at_index(list_box_->get_children().size() - 1);

    entries_.push_back(entry);
    row_to_entry_index_[new_row] = entries_.size() - 1;
}

void remove_entry(const OpenFilesEntry& entry) {

}


}
