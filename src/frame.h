#ifndef DELIMIT_FRAME_H
#define DELIMIT_FRAME_H

#include <gtkmm.h>
#include <gtksourceviewmm.h>

#include <memory>

#include "search.h"

namespace delimit {

class Window;
class Buffer;

class Frame {
public:
    typedef std::shared_ptr<Frame> ptr;

    Frame(Window& parent):
        parent_(parent),
        container_(Gtk::ORIENTATION_VERTICAL),
        file_chooser_(true),
        buffer_(nullptr) {

        build_widgets();
    }

    void set_buffer(Buffer* buffer);
    Buffer* buffer() { return buffer_; }

    Gtk::Box& _gtk_box() { return container_; }
    void set_search_visible(bool value);

private:
    void build_widgets();
    void file_changed_outside_editor(
        const Glib::RefPtr<Gio::File>& file,
        const Glib::RefPtr<Gio::File>& other_file,
        Gio::FileMonitorEvent event
    );

    Window& parent_;

    Gtk::Box container_;
    Gtk::Box file_chooser_box_;
    Gtk::ComboBox file_chooser_;
    Gtk::ScrolledWindow scrolled_window_;
    Gsv::View source_view_;

    delimit::Search search_;

    Buffer* buffer_;
};

}

#endif // FRAME_H
