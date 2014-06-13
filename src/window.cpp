#include <gtkmm.h>
#include <glibmm/i18n.h>
#include <gdkmm.h>

#include "autocomplete/provider.h"
#include "window.h"
#include "application.h"

#include <kazbase/exceptions.h>
#include <kazbase/unicode.h>
#include <kazbase/fdo/base_directory.h>
#include <kazbase/logging.h>
#include <kazbase/glob.h>
#include <kazbase/file_utils.h>

namespace delimit {

const int FRAME_LIMIT = 2;
const unicode UI_FILE = "delimit/ui/delimit.glade";

Window::Window():
    gtk_window_(nullptr),
    gtk_container_(nullptr),
    file_tree_scrolled_window_(nullptr),
    window_file_tree_(nullptr),
    open_file_list_(nullptr),
    buffer_new_(nullptr),
    buffer_open_(nullptr),
    folder_open_(nullptr),
    buffer_save_(nullptr),
    buffer_undo_(nullptr),
    buffer_close_(nullptr),
    main_paned_(nullptr),
    type_(WINDOW_TYPE_FILE),
    current_frame_(0) {

    completion_provider_ = Glib::RefPtr<Provider>(new Provider(this));

    L_DEBUG("Creating window with empty buffer");

    file_tree_store_ = Gtk::TreeStore::create(file_tree_columns_);
    open_list_store_ = Gtk::ListStore::create(open_list_columns_);

    build_widgets();
    new_buffer("Untitled");

    //Don't show the folder tree on FILE windows
    file_tree_scrolled_window_->get_parent()->remove(*file_tree_scrolled_window_);
    open_file_list_->set_vexpand(true);
}

Window::Window(const std::vector<Glib::RefPtr<Gio::File>>& files):
    gtk_window_(nullptr),
    gtk_container_(nullptr),
    file_tree_scrolled_window_(nullptr),
    window_file_tree_(nullptr),
    open_file_list_(nullptr),
    buffer_new_(nullptr),
    buffer_open_(nullptr),
    folder_open_(nullptr),
    buffer_save_(nullptr),
    buffer_undo_(nullptr),
    buffer_close_(nullptr),
    main_paned_(nullptr),
    type_(WINDOW_TYPE_FILE),
    current_frame_(0) {

    completion_provider_ = Glib::RefPtr<Provider>(new Provider(this));

    file_tree_store_ = Gtk::TreeStore::create(file_tree_columns_);
    open_list_store_ = Gtk::ListStore::create(open_list_columns_);

    build_widgets();

    if(files.size() == 1 && files[0]->query_file_type() == Gio::FILE_TYPE_DIRECTORY) {
        type_ = WINDOW_TYPE_FOLDER;

        rebuild_file_tree(files[0]->get_path());
        path_ = files[0]->get_path();

        //Look for a .gitignore file in the directory
        auto ignore_file = os::path::join(path_, ".gitignore");
        L_DEBUG(_u("Checking for .gitignore at {0}...").format(ignore_file));
        if(os::path::exists(ignore_file)) {
            auto lines = file_utils::read_lines(ignore_file);
            for(auto line: lines) {
                if(!line.strip().empty()) {
                    L_DEBUG(_u("Adding ignore glob: '{0}', length: {1}").format(line.strip(), line.strip().length()));
                    ignored_globs_.insert(line.strip());
                }
            }
        } else {
            L_DEBUG("not found.");
        }

        new_buffer("Untitled");

    } else {
        type_ = WINDOW_TYPE_FILE;

        for(auto file: files) {
            open_buffer(file);
        }

        //Don't show the folder tree on FILE windows
        file_tree_scrolled_window_->get_parent()->remove(*file_tree_scrolled_window_);
        open_file_list_->set_vexpand(true);
    }
}

void Window::init_actions() {
    accel_group_ = Gtk::AccelGroup::create();
    buffer_new_->add_accelerator("clicked", accel_group_, GDK_KEY_N, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    buffer_open_->add_accelerator("clicked", accel_group_, GDK_KEY_O, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    folder_open_->add_accelerator("clicked", accel_group_, GDK_KEY_O, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
    buffer_save_->add_accelerator("clicked", accel_group_, GDK_KEY_S, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    window_split_->add_accelerator("clicked", accel_group_, GDK_KEY_T, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    buffer_close_->add_accelerator("clicked", accel_group_, GDK_KEY_W, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

    _gtk_window().add_accel_group(accel_group_);

    action_group_ = Gtk::ActionGroup::create("MainActionGroup");

    add_global_action("search",
        Gtk::AccelKey(GDK_KEY_F, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK),
        std::bind(&Window::begin_search, this)
    );

    add_global_action("find", Gtk::AccelKey(GDK_KEY_F, Gdk::CONTROL_MASK), [&]() {
        frames_[current_frame_]->set_search_visible(true);
    });

    add_global_action("awesome", Gtk::AccelKey(GDK_KEY_K, Gdk::CONTROL_MASK), [&]() {
        frames_[current_frame_]->show_awesome_bar();
    });

    add_global_action("back_up", Gtk::AccelKey(GDK_KEY_Escape, Gdk::ModifierType(0)), [&]() {
        if(frames_[current_frame_]->is_awesome_bar_visible()) {
            frames_[current_frame_]->show_awesome_bar(false);
        } else {
            frames_[current_frame_]->set_search_visible(false);
        }
    });
}

void Window::begin_search() {
    gtk_search_window_->set_transient_for(*gtk_window_);
    int response = gtk_search_window_->run();
    gtk_search_window_->hide();
}

void Window::add_global_action(const unicode& name, const Gtk::AccelKey& key, std::function<void ()> func) {
    auto new_action = Gtk::Action::create(name.encode());
    new_action->signal_activate().connect(func);
    action_group_->add(new_action, key);
    new_action->set_accel_group(accel_group_);
    new_action->connect_accelerator();
}

Glib::RefPtr<Gsv::CompletionProvider> Window::completion_provider() {
    return completion_provider_;
}

void Window::build_widgets() {
    std::string ui_file = fdo::xdg::find_data_file(UI_FILE).encode();
    auto builder = Gtk::Builder::create_from_file(ui_file);

    builder->get_widget("search_window", gtk_search_window_);
    builder->get_widget("main_window", gtk_window_);

    builder->get_widget("window_container", gtk_container_);
    builder->get_widget("window_file_tree", window_file_tree_);
    builder->get_widget("file_tree_scrolled_window", file_tree_scrolled_window_);

    window_file_tree_->set_model(file_tree_store_);
    //window_file_tree_->append_column("Icon", file_tree_columns_.image);
    window_file_tree_->append_column("Name", file_tree_columns_.name);
    window_file_tree_->signal_row_activated().connect(sigc::mem_fun(this, &Window::on_signal_row_activated));
    window_file_tree_->signal_test_expand_row().connect(sigc::mem_fun(this, &Window::on_tree_test_expand_row));

    builder->get_widget("open_file_list", open_file_list_);
    open_file_list_->set_model(open_list_store_);
    open_file_list_->append_column("Name", open_list_columns_.name);
    open_file_list_->signal_row_activated().connect(sigc::mem_fun(this, &Window::on_list_signal_row_activated));

    builder->get_widget("buffer_new", buffer_new_);
    builder->get_widget("buffer_open", buffer_open_);
    builder->get_widget("folder_open", folder_open_);
    builder->get_widget("buffer_save", buffer_save_);
    builder->get_widget("buffer_undo", buffer_undo_);
    builder->get_widget("buffer_redo", buffer_redo_);
    builder->get_widget("window_split", window_split_);
    builder->get_widget("buffer_close", buffer_close_);
    builder->get_widget("error_counter", error_counter_);
    builder->get_widget("window_pane", main_paned_);

    buffer_new_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_new_clicked));
    buffer_open_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_open_clicked));
    buffer_save_->signal_clicked().connect(sigc::hide_return(sigc::mem_fun(this, &Window::toolbutton_save_clicked)));
    folder_open_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_open_folder_clicked));
    buffer_close_->signal_clicked().connect(sigc::mem_fun(this, &Window::close_active_buffer));
    buffer_undo_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_undo_clicked));
    buffer_redo_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_redo_clicked));

    window_split_->signal_toggled().connect([&]() {
        if(window_split_->get_active()) {
            split();
        } else {
            unsplit();
        }
    });

    buffer_new_->set_icon_name("document-new");
    buffer_save_->set_icon_name("document-save");
    buffer_open_->set_icon_name("document-open");
    folder_open_->set_icon_name("folder-open");
    buffer_undo_->set_icon_name("edit-undo");
    buffer_redo_->set_icon_name("edit-redo");
    window_split_->set_icon_name("window-new");

    assert(gtk_window_);

    create_frame(); //Create the default frame

    init_actions();

    //Style the error counter
    Glib::ustring style = "GtkLabel#error_counter { color: white; background-color: #ff0000; border-radius: 3px; font-weight: bold; }";
    auto css = Gtk::CssProvider::create();
    if(css->load_from_data(style)) {
        auto screen = Gdk::Screen::get_default();
        auto ctx = error_counter_->get_style_context();
        ctx->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } else {
        L_ERROR("Unable to set the correct style for the error counter");
    }

    error_counter_->set_no_show_all();
    set_error_count(0);

    std::string icon_file = fdo::xdg::find_data_file("delimit/delimit.svg").encode();
    gtk_window_->set_icon_from_file(icon_file);

    gtk_window_->maximize();

    //Set the pane to be 1/5th the size of the screen width - can't figure out
    //how to work out what the actual size of the window will be :(
    int window_width = Gdk::Screen::get_default()->get_width();
    main_paned_->set_position(window_width / 5);
}

void Window::on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    auto row = *(file_tree_store_->get_iter(path));

    if(!row[file_tree_columns_.is_folder]) {
        Glib::ustring full_path = row[file_tree_columns_.full_path];

        if(Gio::File::create_for_path(full_path)->query_file_type() == Gio::FILE_TYPE_REGULAR) {
            open_buffer(Gio::File::create_for_path(os::path::real_path(full_path.c_str()).encode()));
        }
    }
}

void Window::on_list_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    auto row = *(open_list_store_->get_iter(path));

    Buffer::ptr buffer = row[open_list_columns_.buffer];
    activate_buffer(buffer);
}

void Window::toolbutton_new_clicked() {
    new_buffer("Untitled");
}

void Window::toolbutton_undo_clicked() {
    if(current_frame_ >= 0 && current_frame_ < (int32_t) frames_.size()) {
        frames_.at(current_frame_)->buffer()->_gtk_buffer()->undo();
    }
}

void Window::toolbutton_redo_clicked() {
    if(current_frame_ >= 0 && current_frame_ < (int32_t) frames_.size()) {
        frames_.at(current_frame_)->buffer()->_gtk_buffer()->redo();
    }
}

void Window::toolbutton_open_clicked() {
    Gtk::FileChooserDialog dialog(_gtk_window(), _("Open a file..."));

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);

    int result = dialog.run();
    switch(result) {
        case Gtk::RESPONSE_OK: {
            std::string filename = dialog.get_filename();
            open_buffer(Gio::File::create_for_path(filename));
        } default:
            break;
    }
}

void Window::toolbutton_open_folder_clicked() {
    Gtk::FileChooserDialog dialog(_("Open a folder..."), Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    int result = dialog.run();
    switch(result) {
        case Gtk::RESPONSE_OK: {
            std::string filename = dialog.get_filename();

            Gio::Application::type_vec_files files;
            files.push_back(Gio::File::create_for_path(filename));
            auto window = std::make_shared<delimit::Window>(files);
            Glib::RefPtr<Gtk::Application> app = gtk_window_->get_application();
            Glib::RefPtr<Application>::cast_dynamic(app)->add_window(window);
        } default:
            break;
    }
}

bool Window::toolbutton_save_clicked() {
    bool was_saved = false;

    Gtk::FileChooserDialog dialog(_gtk_window(), _("Save a file"), Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_do_overwrite_confirmation(true);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    Buffer* buffer = frames_[current_frame_]->buffer();
    assert(buffer);

    if(buffer) {
        unicode path = buffer->path();
        if(path.empty()) {
            int result = dialog.run();
            switch(result) {
                case Gtk::RESPONSE_OK:
                    path = dialog.get_filename();
                    break;
                default:
                    return was_saved;
            }
        }
        buffer->save(path);
        was_saved = true;
    }

    return was_saved;
}


bool Window::on_tree_test_expand_row(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path) {
    Gtk::TreeRow row = (*iter);

    //Remove all dummy rows
    std::vector<Gtk::TreeRowReference> dummy_nodes;
    for(auto child: row->children()) {
        if(child[file_tree_columns_.is_dummy]) {
            dummy_nodes.push_back(Gtk::TreeRowReference(file_tree_store_, file_tree_store_->get_path(child)));
        }
    }

    if(!dummy_nodes.empty()) {
        //Build the node
        auto path = Glib::ustring(row[file_tree_columns_.full_path]);
        auto unicode_path = unicode(path.c_str());
        dirwalk(unicode_path, &row);

        for(auto ref: dummy_nodes) {
            file_tree_store_->erase(file_tree_store_->get_iter(ref.get_path()));//Erase the dummy
        }
    }

    return false;
}

void Window::set_error_count(int32_t count) {
    if(count == 0) {
        error_counter_->set_text("0");
        error_counter_->hide();
    } else {
        error_counter_->set_text(_u("{0}").format(count).encode());
        error_counter_->show();
    }
}

void Window::on_folder_changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &other, Gio::FileMonitorEvent event_type) {
    L_DEBUG("Detected folder event: " + file->get_path());

    if(event_type == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT || event_type == Gio::FILE_MONITOR_EVENT_DELETED || event_type == Gio::FILE_MONITOR_EVENT_CREATED) {
        L_INFO("Detected folder change: " + file->get_path());

        unicode folder_path = os::path::dir_name(file->get_path());

        Gtk::TreeRow node = *(file_tree_store_->get_iter(tree_row_lookup_.at(folder_path).get_path()));
        dirwalk(folder_path, &node);
    }
}

void Window::watch_directory(const unicode& path) {
    if(tree_monitors_.find(path) != tree_monitors_.end()) {
        return;
    }

    //Handle updates to the directory
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path.encode());
    auto monitor = file->monitor_directory();
    tree_monitors_[path] = monitor;
    monitor->signal_changed().connect(sigc::mem_fun(this, &Window::on_folder_changed));
}

void Window::unwatch_directory(const unicode& path) {
    if(tree_monitors_.find(path) == tree_monitors_.end()) {
        return;
    }

    Glib::RefPtr<Gio::FileMonitor> monitor = tree_monitors_.at(path);
    monitor->cancel();
    tree_monitors_.erase(path);
}

void Window::dirwalk(const unicode& path, const Gtk::TreeRow* node) {
    std::vector<unicode> files;

    bool path_is_dir = os::path::is_dir(os::path::real_path(path));
    if(!path_is_dir) {
        L_DEBUG("Not walking file: " + path.encode());
        return;
    }

    watch_directory(path);

    files = os::path::list_dir(os::path::real_path(path));
    std::sort(files.begin(), files.end());

    std::vector<unicode> directories;

    for(auto it = files.begin(); it != files.end();) {
        unicode f = (*it);
        unicode real_path = os::path::real_path(os::path::join(path, f));

        if(real_path.empty()) {
            //Broken symlink or something
            it = files.erase(it);
            L_DEBUG(_u("Skipping broken file {0}").format(f));
            continue;
        }

        L_DEBUG(_u("Adding file: {0}").format(real_path));
        if(os::path::is_dir(real_path)) {
            L_DEBUG(_u("...Is directory"));
            directories.push_back((*it));
            it = files.erase(it);
        } else {
            ++it;
        }
    }

    directories.insert(directories.end(), files.begin(), files.end());
    files = directories;

    Gtk::TreeIter root_iter;

    if(!node) {
        L_DEBUG("Adding root node: " + path.encode());
        auto image = Gtk::IconTheme::get_default()->load_icon("folder", Gtk::ICON_SIZE_MENU);

        //If the node already exists for this path, don't add it again
        if(tree_row_lookup_.find(path) != tree_row_lookup_.end()) {
            root_iter = file_tree_store_->get_iter(tree_row_lookup_[path].get_path());
        } else {
            //Add a new node, store a reference against the path
            root_iter = file_tree_store_->append();
            tree_row_lookup_[path] = Gtk::TreeRowReference(file_tree_store_, file_tree_store_->get_path(root_iter));
        }

        node = &(*root_iter);
        node->set_value(file_tree_columns_.name, Glib::ustring(os::path::split(path).second.encode()));
        node->set_value(file_tree_columns_.full_path, Glib::ustring(path.encode()));
        node->set_value(file_tree_columns_.image, image);
        node->set_value(file_tree_columns_.is_folder, true);
        node->set_value(file_tree_columns_.is_dummy, false);
    }


    std::vector<unicode> existing_children;

    auto it = tree_row_lookup_.find(path);
    if(it != tree_row_lookup_.end()) {
        //This was an existing folder in the tree, so it might have children

        auto ref = (*it).second;
        for(auto child: (*file_tree_store_->get_iter(ref.get_path())).children()) {
            //Go through each child node, store any existing children
            if(!child->get_value(file_tree_columns_.is_dummy)) {
                //this isn't a dummy, so add the path to the existing_children
                existing_children.push_back(unicode(child->get_value(file_tree_columns_.full_path)));
            }
        }
    }

    for(auto f: files) {
        if(f == "." || f == "..") continue;

        if(f.starts_with(".") || f.ends_with(".pyc")) {
            //Ignore hidden files and folders
            continue;
        }

        unicode full_name = os::path::join(path, f);

        //We've found one of the existing children, so remove it from the list
        existing_children.erase(std::remove(existing_children.begin(), existing_children.end(), full_name), existing_children.end());

        bool ignore = false;
        for(auto gl: ignored_globs_) {
            if(glob::match(full_name, gl)) {
                //Ignore this file/folder if it matches the ignored globs
                ///// DISABLED UNTIL I FIGURE OUT WHAT BROKE ignore = true;
                break;
            }
        }

        if(ignore) {
            L_DEBUG("Ignoring file as it's in .gitignore or similar: " + full_name.encode());
            continue;
        }

        bool is_folder = os::path::is_dir(os::path::real_path(full_name));

        auto image = Gtk::IconTheme::get_default()->load_icon(
            (is_folder) ? "folder" : "document",
            Gtk::ICON_SIZE_MENU
        );

        Gtk::TreeIter iter;
        const Gtk::TreeRow* row = nullptr;

        //If the node already exists for this path, don't add it again
        if(tree_row_lookup_.find(full_name) != tree_row_lookup_.end()) {
            iter = file_tree_store_->get_iter(tree_row_lookup_[full_name].get_path());
        } else {
            if(node) {
                L_DEBUG("Adding child node: " + full_name.encode());
                iter = file_tree_store_->append(node->children());
            } else {
                L_DEBUG("Adding root node: " + full_name.encode());
                iter = file_tree_store_->append();
            }
            tree_row_lookup_[full_name] = Gtk::TreeRowReference(file_tree_store_, file_tree_store_->get_path(iter));
        }

        row = &(*iter);
        row->set_value(file_tree_columns_.name, Glib::ustring(f.encode()));
        row->set_value(file_tree_columns_.full_path, Glib::ustring(full_name.encode()));
        row->set_value(file_tree_columns_.image, image);
        row->set_value(file_tree_columns_.is_folder, is_folder);
        row->set_value(file_tree_columns_.is_dummy, false);

        if(is_folder && row->children().empty()) {
            //Add a dummy node under the folder
            const Gtk::TreeRow* dummy = &(*file_tree_store_->append(row->children()));
            dummy->set_value(file_tree_columns_.is_dummy, true);
        }
    }

    for(unicode file: existing_children) {
        L_DEBUG("Found file in need of removal: " + file.encode());
        file_tree_store_->erase(file_tree_store_->get_iter(tree_row_lookup_.at(file).get_path()));
        tree_row_lookup_.erase(file);
        unwatch_directory(file);
    }
}

void Window::rebuild_file_tree(const unicode& path) {
    file_tree_store_->clear();

    dirwalk(path, nullptr);

    window_file_tree_->expand_row(Gtk::TreeModel::Path("0"), false);
}

void Window::rebuild_open_list() {
    open_list_store_->clear();

    auto tmp = open_buffers_;

    std::sort(tmp.begin(), tmp.end(), [](Buffer::ptr lhs, Buffer::ptr rhs) {
        return lhs->name().lower() < rhs->name().lower();
    });

    std::map<unicode, int> occurrance_count;

    for(auto buffer: tmp) {
        unicode name = buffer->name();
        if(occurrance_count.find(name) == occurrance_count.end()) {
            occurrance_count.insert(std::make_pair(name, 0));
        }

        occurrance_count[name] += 1;
    }

    for(auto buffer: tmp) {
        auto row = *(open_list_store_->append());

        unicode name = buffer->name();

        if(occurrance_count[name] > 1) {
            name = _u("{0} ({1})").format(name, os::path::dir_name(buffer->path().replace(path_, "").lstrip("/")));
        }

        row[open_list_columns_.name] = Glib::ustring(name.encode());
        row[open_list_columns_.buffer] = buffer;
    }
}


void Window::split() {
    assert(frames_.size() == 1);
    create_frame();
}

void Window::unsplit() {
    assert(frames_.size() == 2);

    gtk_container_->remove();
    frames_[0]->_gtk_box().reparent(*gtk_container_);
    gtk_container_->show_all();

    frames_.pop_back(); //Destroy the second frame
}

void Window::on_buffer_modified(Buffer* buffer) {
    buffer_save_->set_sensitive(buffer->modified());

    unicode to_display = (buffer->path().empty()) ? buffer->name() : buffer->path();

    if(type_ == WINDOW_TYPE_FOLDER && !path_.empty() && !buffer->path().empty()) {
        to_display = os::path::rel_path(to_display, path_);
    }

    _gtk_window().set_title(
        _u("Delimit - {0}{1}").format(
            to_display, buffer->modified() ? "*": ""
        ).encode().c_str()
    );
}

void Window::activate_buffer(Buffer::ptr buffer) {
    assert(current_frame_ >= 0 && current_frame_ < (int32_t) frames_.size());
    frames_[current_frame_]->set_buffer(buffer.get());
    on_buffer_modified(buffer.get());
}

void Window::new_buffer(const unicode& name) {
    Buffer::ptr buffer = std::make_shared<Buffer>(*this, name);

    //Watch for changes to the buffer
    buffer->signal_modified_changed().connect(
        sigc::mem_fun(this, &Window::on_buffer_modified)
    );

    buffer->signal_closed().connect(
        sigc::mem_fun(this, &Window::close_buffer)
    );

    buffer->set_modified(true); //New file, mark as modified

    open_buffers_.push_back(buffer);

    activate_buffer(buffer);
    rebuild_open_list();
}

void Window::close_active_buffer() {
    frames_[current_frame_]->buffer()->close();
}

void Window::close_buffer(Buffer* buffer) {
    Buffer::ptr prev, this_buf;
    for(uint32_t i = 0; i < open_buffers_.size(); ++i) {
        auto buf = open_buffers_[i];

        if(buf.get() == buffer) {
            this_buf = buf;
            if(!prev && i + 1 < open_buffers_.size()) {
                //We closed the first file in the list, so point to the following one
                //if we can
                prev = open_buffers_[i + 1];
            }
            break;
        }

        prev = buf;
    }

    if(prev) {
        activate_buffer(prev);
    }

    //erase the buffer from the open buffers
    open_buffers_.erase(std::remove(open_buffers_.begin(), open_buffers_.end(), this_buf), open_buffers_.end());

    //Make sure we always have a document
    if(open_buffers_.empty()) {
        //Close the window... maybe revisit this
        //FIXME: Gtk 3.10 supports actual closing rather than ugly killing
//        gtk_window_->close();
        gtk_widget_destroy(GTK_WIDGET(gtk_window_->gobj()));
        return;
    }

    rebuild_open_list();
}

void Window::close_buffer_for_file(const Glib::RefPtr<Gio::File>& file) {
    for(auto buffer: open_buffers_) {
        if(buffer->path() == file->get_path()) {
            buffer->close();
        }
    }
}

void Window::open_buffer(const Glib::RefPtr<Gio::File> &file) {
    if(!file->query_exists()) {
        throw IOError(_u("Path does not exist: {0}").format(file->get_path()));
    }

    for(auto buffer: open_buffers_) {
        if(buffer->path() == file->get_path()) {
            //Just activate, don't open!
            activate_buffer(buffer);
            return;
        }
    }

    unicode name = os::path::split(file->get_path()).second;

    Buffer::ptr buffer = std::make_shared<Buffer>(*this, name, file);
    buffer->set_modified(false);
    //Watch for changes to the buffer
    buffer->signal_modified_changed().connect(
        sigc::mem_fun(this, &Window::on_buffer_modified)
    );

    buffer->signal_closed().connect(
        sigc::mem_fun(this, &Window::close_buffer)
    );

    open_buffers_.push_back(buffer);

    activate_buffer(buffer);
    rebuild_open_list();
}

void Window::create_frame() {
    assert(frames_.size() < (uint32_t) FRAME_LIMIT);

    Frame::ptr frame = std::make_shared<Frame>(*this);

    if(frames_.empty()) {
        gtk_container_->add(frame->_gtk_box());
    } else if(frames_.size() == 1) {
        Gtk::Paned* pane = Gtk::manage(new Gtk::Paned);
        gtk_container_->remove();
        gtk_container_->add(*pane);

        pane->show_all();

        pane->add1(frames_[0]->_gtk_box());
        pane->add2(frame->_gtk_box());

        frame->set_buffer(frames_[0]->buffer());
    } else {
        throw std::logic_error("More than two frames not supported");
    }

    frame->_gtk_box().show_all();
    frames_.push_back(frame);
}

void Window::set_undo_enabled(bool value) {
    buffer_undo_->set_sensitive(value);
}

void Window::set_redo_enabled(bool value) {
    buffer_redo_->set_sensitive(value);
}


}
