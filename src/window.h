#ifndef DELIMIT_WINDOW_H
#define DELIMIT_WINDOW_H

#include <map>
#include <vector>

#include "base/unicode.h"
#include <gtkmm.h>

#include "buffer.h"
#include "frame.h"

namespace delimit {

enum WindowType {
    WINDOW_TYPE_FOLDER,
    WINDOW_TYPE_FILE
};

class Window {
public:
    typedef std::shared_ptr<Window> ptr;

    Window();
    Window(const std::vector<Glib::RefPtr<Gio::File>>& files);

    void split();
    void unsplit();

    void new_buffer(const unicode& name);
    void open_buffer(const Glib::RefPtr<Gio::File>& path);

    WindowType type() const { return type_; }

    Gtk::Window& _gtk_window() { assert(gtk_window_); return *gtk_window_; }

private:
    void build_widgets();

    Gtk::Window* gtk_window_ = nullptr;
    Gtk::Alignment* gtk_container_ = nullptr;
    Gtk::TreeView* window_file_tree_ = nullptr;

    //Toolbar
    Gtk::ToolButton* buffer_undo_ = nullptr;
    Gtk::ToolButton* buffer_redo_ = nullptr;

    void create_frame();

    WindowType type_ = WINDOW_TYPE_FILE;
    unicode path_;

    std::vector<Buffer::ptr> open_buffers_;
    std::vector<Frame::ptr> frames_;
    int32_t current_frame_;
};

}

#endif // WINDOW_H
