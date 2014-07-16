#ifndef DELIMIT_WINDOW_H
#define DELIMIT_WINDOW_H

#include <map>
#include <vector>
#include <set>
#include <cassert>

#include <kazbase/unicode.h>
#include <kazbase/json/json.h>

#include <gtkmm.h>

#include "search/search_thread.h"
#include "buffer.h"
#include "frame.h"

namespace delimit {

class Provider;

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

    void new_buffer();
    void open_buffer(const Glib::RefPtr<Gio::File>& path);

    WindowType type() const { return type_; }

    Gtk::Window& _gtk_window() { assert(gtk_window_); return *gtk_window_; }

    void set_undo_enabled(bool value);
    void set_redo_enabled(bool value);

    void rebuild_file_tree(const unicode &path);
    void rebuild_open_list();

    bool toolbutton_save_clicked();

    unicode project_path() const { return path_; }

    void set_error_count(int32_t count);

    int new_file_count() const;

    const json::JSON settings() { return settings_; }
private:
    void load_settings();

    void close_buffer(Buffer* buffer);
    void close_buffer_for_file(const Glib::RefPtr<Gio::File>& file);
    void close_active_buffer();

    void activate_buffer(Buffer::ptr buffer);
    void build_widgets();
    void init_actions();

    void dirwalk(const unicode& path, const Gtk::TreeRow *node);
    void watch_directory(const unicode& path);
    void unwatch_directory(const unicode& path);

    bool on_tree_test_expand_row(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path);

    void on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_list_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_buffer_modified(Buffer* buffer);

    void begin_search();

    Gtk::Dialog* gtk_search_window_;
    std::shared_ptr<SearchThread> search_thread_;


    Gtk::ApplicationWindow* gtk_window_;
    Gtk::Alignment* gtk_container_;

    Gtk::ScrolledWindow* file_tree_scrolled_window_;
    Gtk::TreeView* window_file_tree_;
    Gtk::TreeView* open_file_list_;

    FileTreeColumns file_tree_columns_;
    Glib::RefPtr<Gtk::TreeStore> file_tree_store_;

    OpenListColumns open_list_columns_;
    Glib::RefPtr<Gtk::ListStore> open_list_store_;

    Gtk::HeaderBar header_bar_;

    //Toolbar
    Gtk::ToolButton* buffer_new_;
    Gtk::ToolButton* buffer_open_;
    Gtk::ToolButton* folder_open_;
    Gtk::ToolButton* buffer_save_;
    Gtk::ToolButton* buffer_undo_;
    Gtk::ToolButton* buffer_redo_;
    Gtk::ToggleToolButton* window_split_;
    Gtk::Paned* main_paned_;

    Gtk::Button* buffer_close_;
    Gtk::Label* error_counter_;

    Glib::RefPtr<Gtk::ActionGroup> actions_;

    void toolbutton_new_clicked();
    void toolbutton_open_clicked();
    void toolbutton_open_folder_clicked();

    void toolbutton_undo_clicked();
    void toolbutton_redo_clicked();

    void create_frame();

    WindowType type_;
    unicode path_;

    std::vector<Buffer::ptr> open_buffers_;
    std::vector<Frame::ptr> frames_;
    int32_t current_frame_;

    std::set<unicode> ignored_globs_; //For file tree

    std::map<unicode, Glib::RefPtr<Gio::FileMonitor> > tree_monitors_;
    std::map<unicode, Gtk::TreeRowReference> tree_row_lookup_;

    void on_folder_changed(const Glib::RefPtr<Gio::File>& file, const Glib::RefPtr<Gio::File>& other, Gio::FileMonitorEvent event_type);

    Glib::RefPtr<Gtk::AccelGroup> accel_group_;
    Glib::RefPtr<Gtk::ActionGroup> action_group_;

    void add_global_action(const unicode& name, const Gtk::AccelKey& key, std::function<void ()> func);

    json::JSON settings_;
};

}

#endif // WINDOW_H
