#ifndef DELIMIT_FRAME_H
#define DELIMIT_FRAME_H

#include <gtkmm.h>
#include <gtksourceviewmm.h>

#include <memory>

namespace delimit {

class Window;
class Buffer;

class Frame {
public:
    typedef std::shared_ptr<Frame> ptr;

    Frame(Window& parent):
        parent_(parent),
        container_(Gtk::ORIENTATION_VERTICAL),
        file_chooser_(true) {

        build_widgets();
    }

    void set_buffer(Buffer* buffer);

    Gtk::Box& _gtk_box() { return container_; }

private:
    void build_widgets();

    Window& parent_;

    Gtk::Box container_;
    Gtk::Box file_chooser_box_;
    Gtk::ComboBox file_chooser_;
    Gtk::ScrolledWindow scrolled_window_;
    Gsv::View source_view_;

    Buffer* buffer_ = nullptr;
};

}

#endif // FRAME_H
