#ifndef DELIMIT_FRAME_H
#define DELIMIT_FRAME_H

#include <gtkmm.h>
#include <gtksourceviewmm.h>
#include <kazbase/json/json.h>
#include <memory>

#include "awesome_bar.h"
#include "search.h"

#include "autocomplete/provider.h"

namespace delimit {

class Window;
class Buffer;

class Frame {
public:
    typedef std::shared_ptr<Frame> ptr;

    Frame(Window& parent);
    void set_buffer(Buffer* buffer);
    Buffer* buffer() { return buffer_; }
    Gsv::View& view() { return source_view_; }


    Gtk::Box& _gtk_box() { return container_; }

    sigc::signal<void, Buffer*>& signal_buffer_changed() { return signal_buffer_changed_; }
private:
    void build_widgets();

    Window& parent_;

    Gtk::Box container_;
    Gtk::Box file_chooser_box_;
    Gtk::ComboBox file_chooser_;
    Gtk::ScrolledWindow scrolled_window_;
    Gsv::View source_view_;



    Buffer* buffer_;

    sigc::signal<void, Buffer*> signal_buffer_changed_;
    sigc::connection file_changed_connection_;
    sigc::connection buffer_changed_connection_;
    sigc::connection buffer_loaded_connection_;

    Glib::RefPtr<Provider> provider_;

    void check_undoable_actions();

    void apply_settings(const unicode& mimetype);
    void detect_and_apply_indentation(Buffer* buffer);

    json::JSON current_settings_;
};

}

#endif // FRAME_H
