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
        add(is_dummy);
    }

    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Glib::ustring> full_path;
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> image;
    Gtk::TreeModelColumn<bool> is_folder;
    Gtk::TreeModelColumn<bool> is_dummy;
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
    void close_buffer(Buffer* buffer);
    void close_buffer(const Glib::RefPtr<Gio::File>& file);

    WindowType type() const { return type_; }

    Gtk::Window& _gtk_window() { assert(gtk_window_); return *gtk_window_; }

    void new_buffer_thing() {
        new_buffer("Thing");
    }

    Gtk::ToggleToolButton& buffer_search_button() { return *buffer_search_; }

private:
    void activate_buffer(Buffer::ptr buffer);
    void build_widgets();
    void init_actions();
    void rebuild_file_tree(const unicode &path);
    void rebuild_open_list();

    void dirwalk(const unicode& path, const Gtk::TreeRow *node);

    bool on_tree_test_expand_row(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path);

    void on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_list_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_buffer_modified(Buffer::ptr buffer);

    Gtk::ApplicationWindow* gtk_window_;
    Gtk::Alignment* gtk_container_;

    Gtk::ScrolledWindow* file_tree_scrolled_window_;
    Gtk::TreeView* window_file_tree_;
    Gtk::TreeView* open_file_list_;

    FileTreeColumns file_tree_columns_;
    Glib::RefPtr<Gtk::TreeStore> file_tree_store_;

    OpenListColumns open_list_columns_;
    Glib::RefPtr<Gtk::ListStore> open_list_store_;

    //Toolbar
    Gtk::ToolButton* buffer_new_;
    Gtk::ToolButton* buffer_open_;
    Gtk::ToolButton* buffer_save_;
    Gtk::ToolButton* buffer_undo_;
    Gtk::ToolButton* buffer_redo_;
    Gtk::ToggleToolButton* buffer_search_;
    Gtk::ToggleToolButton* window_split_;

    Glib::RefPtr<Gtk::ActionGroup> actions_;

    void toolbutton_new_clicked();
    void toolbutton_open_clicked();
    void toolbutton_save_clicked();
    void toolbutton_search_toggled();

    void create_frame();

    WindowType type_;
    unicode path_;

    std::vector<Buffer::ptr> open_buffers_;
    std::vector<Frame::ptr> frames_;
    int32_t current_frame_;
};

}

#endif // WINDOW_H
