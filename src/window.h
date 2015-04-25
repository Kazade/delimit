#ifndef DELIMIT_WINDOW_H
#define DELIMIT_WINDOW_H

#include <map>
#include <vector>
#include <set>
#include <cassert>

#include <kazbase/unicode.h>
#include <kazbase/json/json.h>

#include <gtkmm.h>

#include "awesome_bar.h"
#include "search/search_thread.h"
#include "find_bar.h"
#include "document_view.h"
#include "gtk/open_files_list.h"

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

class ProjectInfo;

class FileTree : public Gtk::TreeView {
public:
    FileTree(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder):
        Gtk::TreeView(cobject) {

        Gtk::MenuItem* find_item = Gtk::manage(new Gtk::MenuItem("Search directory...", true));
        //item->signal_activate().connect(sigc::mem_fun(*this, &TreeView_WithPopup::on_menu_file_popup_generic) );
        find_item->signal_activate().connect([=]() {
            auto selection = get_selection();
            if(selection) {
                auto iter = selection->get_selected();
                if(iter) {
                    Glib::ustring path = (*iter)[FileTreeColumns().full_path];
                    signal_search_directory_(path);
                }
            }
        });

        folder_menu_.append(*find_item);

        Gtk::MenuItem* delete_item = Gtk::manage(new Gtk::MenuItem("_Delete", true));
        file_menu_.append(*delete_item);

        Gtk::MenuItem* rename_item = Gtk::manage(new Gtk::MenuItem("_Rename", true));
        file_menu_.append(*rename_item);

        file_menu_.show_all();
        folder_menu_.show_all();
    }

    bool on_button_press_event(GdkEventButton* event) {
        bool return_value = false;

        //Call base class, to allow normal handling,
        //such as allowing the row to be selected by the right-click:
        return_value = TreeView::on_button_press_event(event);

        //Then do our custom stuff:
        if((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
            auto selection = get_selection();
            if(selection) {
                auto iter = selection->get_selected();
                if(iter) {
                    bool is_folder = (*iter)[FileTreeColumns().is_folder];
                    if(is_folder) {
                        folder_menu_.popup(event->button, event->time);
                    } else {
                        file_menu_.popup(event->button, event->time);
                    }
                }
            }

        }

        return return_value;
    }

    sig::signal<void (std::string)>& signal_search_directory() { return signal_search_directory_; }
private:
    Gtk::Menu file_menu_;
    Gtk::Menu folder_menu_;

    sig::signal<void (std::string)> signal_search_directory_;
};


class Window {
public:
    typedef std::shared_ptr<Window> ptr;

    Window();
    Window(const std::vector<Glib::RefPtr<Gio::File>>& files);

    void new_document();
    void open_document(const unicode& path);

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

    DocumentView::ptr current_buffer() { return current_document_; }

    void show_awesome_bar(bool value=true);
    bool is_awesome_bar_visible() const { return awesome_bar_->get_visible(); }

    sigc::signal<void, DocumentView&>& signal_document_switched() { return signal_document_switched_; }

    std::shared_ptr<ProjectInfo> info() { return info_; }

    void set_tasks_visible(bool value=true);
    void set_task_active(uint32_t index);
    void set_task_in_progress(uint32_t index, bool value=true);
    void set_task_tabs_visible(bool value=true);
    void clear_search_results();
private:
    typedef std::function<void ()> TaskCancelFunc;
    struct TaskState {
        TaskState(unicode title, bool in_progress=false, TaskCancelFunc cancel_func=TaskCancelFunc()):
            title(title), in_progress(in_progress), func(cancel_func) {}

        unicode title;
        bool in_progress = false;
        TaskCancelFunc func;
    };

    std::vector<TaskState> task_states_;
    uint32_t active_task_ = 0;

    void close_document(DocumentView &document);
    void append_document(DocumentView::ptr document);

    void load_settings();

    void activate_document(DocumentView::ptr buffer);
    void build_widgets();
    void init_actions();

    void dirwalk(const unicode& path, const Gtk::TreeRow *node);
    void watch_directory(const unicode& path);
    void unwatch_directory(const unicode& path);

    bool on_tree_test_expand_row(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path);

    void on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
    void on_list_signal_row_activated(DocumentView::ptr buffer);
    void on_document_modified(DocumentView &document);

    void begin_search(const std::string& within_directory="");
    void cancel_search();

    Gtk::Dialog* gtk_search_window_;
    std::shared_ptr<SearchThread> search_thread_;
    Gtk::Entry* search_text_entry_;
    Gtk::Entry* matching_glob_;
    Gtk::Entry* excluding_glob_;
    Gtk::FileChooserButton* search_within_;

    Gtk::ApplicationWindow* gtk_window_;
    Gtk::Alignment* gtk_container_;
    Gtk::Alignment* no_files_alignment_;

    Gtk::ScrolledWindow* file_tree_scrolled_window_;
    FileTree* window_file_tree_;


    Gtk::Notebook* tasks_book_;
    Gtk::Button* tasks_hide_button_;
    Gtk::Button* tasks_stop_button_;
    Gtk::Spinner* tasks_progress_;
    Gtk::Box* tasks_header_;
    Gtk::ButtonBox* tasks_buttons_;
    Gtk::Box* search_results_;
    Gtk::Box* tasks_box_;
    sigc::connection tasks_stop_connection_;
    Gtk::Paned* content_pane_;


    FileTreeColumns file_tree_columns_;
    Glib::RefPtr<Gtk::TreeStore> file_tree_store_;

    std::shared_ptr<_Gtk::OpenFilesList> open_files_list_;
    std::vector<_Gtk::OpenFilesEntry> open_files_store_;

    Gtk::HeaderBar header_bar_;

    //Toolbar
    Gtk::ToolButton* buffer_new_;
    Gtk::ToolButton* buffer_open_;
    Gtk::ToolButton* folder_open_;
    Gtk::ToolButton* buffer_save_;
    Gtk::ToolButton* buffer_undo_;
    Gtk::ToolButton* buffer_redo_;
    Gtk::Paned* main_paned_;
    GtkOverlay* overlay_;

    Gtk::Label* error_counter_;

    Glib::RefPtr<Gtk::ActionGroup> actions_;

    void toolbutton_new_clicked();
    void toolbutton_open_clicked();
    void toolbutton_open_folder_clicked();

    void toolbutton_undo_clicked();
    void toolbutton_redo_clicked();

    WindowType type_;
    unicode path_;

    std::vector<DocumentView::ptr> documents_;
    DocumentView::ptr current_document_;

    std::shared_ptr<FindBar> find_bar_;
    std::shared_ptr<AwesomeBar> awesome_bar_;

    std::set<unicode> ignored_globs_; //For file tree

    std::map<unicode, Glib::RefPtr<Gio::FileMonitor> > tree_monitors_;
    std::map<unicode, Gtk::TreeRowReference> tree_row_lookup_;

    void on_folder_changed(const Glib::RefPtr<Gio::File>& file, const Glib::RefPtr<Gio::File>& other, Gio::FileMonitorEvent event_type);

    Glib::RefPtr<Gtk::AccelGroup> accel_group_;
    Glib::RefPtr<Gtk::ActionGroup> action_group_;

    void add_global_action(const unicode& name, const Gtk::AccelKey& key, std::function<void ()> func);

    json::JSON settings_;

    sigc::signal<void, DocumentView&> signal_document_switched_;

    void update_vcs_branch_in_tree();

    std::shared_ptr<ProjectInfo> info_;
};

}

#endif // WINDOW_H
