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

class FileTreeColumns : public Gtk::TreeModelColumnRecord {
public:
    FileTreeColumns() {
        add(name);
        add(full_path);
        add(image);
        add(is_folder);
    }

    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> full_path;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> image;
    Gtk::TreeModelColumn<bool> is_folder;
};

class OpenListColumns : public Gtk::TreeModelColumnRecord {
public:
    OpenListColumns() {
        add(name);
        add(buffer);
    }

    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Buffer::ptr> buffer;
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
    void rebuild_file_tree(const unicode &path);
    void rebuild_open_list();

    void dirwalk(const unicode& path, const Gtk::TreeRow *node);

    void on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_list_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

    Gtk::Window* gtk_window_ = nullptr;
    Gtk::Alignment* gtk_container_ = nullptr;

    Gtk::ScrolledWindow* file_tree_scrolled_window_ = nullptr;
    Gtk::TreeView* window_file_tree_ = nullptr;
    Gtk::TreeView* open_file_list_ = nullptr;

    FileTreeColumns file_tree_columns_;
    Glib::RefPtr<Gtk::TreeStore> file_tree_store_;

    OpenListColumns open_list_columns_;
    Glib::RefPtr<Gtk::ListStore> open_list_store_;

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
