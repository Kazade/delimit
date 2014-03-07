#include <gtkmm.h>
#include <glibmm/i18n.h>
#include <gdkmm.h>

#include "window.h"

#include "base/exceptions.h"
#include "base/unicode.h"
#include "base/fdo/base_directory.h"
#include "base/logging.h"
#include "base/glob.h"
#include "base/file_utils.h"

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
    buffer_save_(nullptr),
    buffer_undo_(nullptr),
    buffer_search_(nullptr),
    buffer_close_(nullptr),
    type_(WINDOW_TYPE_FILE),
    current_frame_(0) {

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
    buffer_save_(nullptr),
    buffer_undo_(nullptr),
    buffer_search_(nullptr),
    buffer_close_(nullptr),
    type_(WINDOW_TYPE_FILE) {

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
    auto accel_group = Gtk::AccelGroup::create();
    buffer_new_->add_accelerator("clicked", accel_group, GDK_KEY_N, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    buffer_save_->add_accelerator("clicked", accel_group, GDK_KEY_S, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    window_split_->add_accelerator("clicked", accel_group, GDK_KEY_T, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    buffer_close_->add_accelerator("clicked", accel_group, GDK_KEY_W, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

    buffer_search_->get_child()->add_accelerator(
        "clicked", accel_group, GDK_KEY_F, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE
    );

    _gtk_window().add_accel_group(accel_group);
}

void Window::build_widgets() {
    std::string ui_file = fdo::xdg::find_data_file(UI_FILE).encode();
    auto builder = Gtk::Builder::create_from_file(ui_file);
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
    builder->get_widget("buffer_save", buffer_save_);
    builder->get_widget("buffer_undo", buffer_undo_);
    builder->get_widget("buffer_redo", buffer_redo_);
    builder->get_widget("buffer_search", buffer_search_);
    builder->get_widget("window_split", window_split_);
    builder->get_widget("buffer_close", buffer_close_);

    buffer_new_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_new_clicked));
    buffer_open_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_open_clicked));
    buffer_save_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_save_clicked));
    buffer_search_->signal_toggled().connect(sigc::mem_fun(this, &Window::toolbutton_search_toggled));
    buffer_close_->signal_clicked().connect(sigc::mem_fun(this, &Window::close_active_buffer));
    buffer_undo_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_undo_clicked));
    buffer_redo_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_redo_clicked));

    assert(gtk_window_);

    create_frame(); //Create the default frame

    init_actions();

    std::string icon_file = fdo::xdg::find_data_file("delimit/delimit.svg").encode();
    gtk_window_->set_icon_from_file(icon_file);

    gtk_window_->maximize();
}

void Window::on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    auto row = *(file_tree_store_->get_iter(path));

    if(!row[file_tree_columns_.is_folder]) {
        Glib::ustring full_path = row[file_tree_columns_.full_path];

        if(Gio::File::create_for_path(full_path)->query_file_type() == Gio::FILE_TYPE_REGULAR) {
            open_buffer(Gio::File::create_for_path(full_path));
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

void Window::toolbutton_save_clicked() {
    Gtk::FileChooserDialog dialog(_gtk_window(), _("Save a file"), Gtk::FILE_CHOOSER_ACTION_SAVE);
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
                    return;
            }
        }
        buffer->save(path);
    }
}


void Window::toolbutton_search_toggled() {
    frames_[current_frame_]->set_search_visible(buffer_search_->get_active());
}

bool Window::on_tree_test_expand_row(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path) {
    Gtk::TreeRow row = (*iter);

    bool is_empty = row->children().empty();
    bool is_dummy = !is_empty && row->children()[0][file_tree_columns_.is_dummy];

    if(is_dummy) {
        //this node contains a dummy, so delete it
        Gtk::TreeIter to_remove = row->children()[0];

        //Then build the node
        dirwalk(Glib::ustring(row[file_tree_columns_.full_path]).c_str(), &row);

        file_tree_store_->erase(to_remove); //Erase the dummy
    }

    return false;
}

void Window::dirwalk(const unicode& path, const Gtk::TreeRow* node) {
    auto files = os::path::list_dir(os::path::real_path(path));
    std::sort(files.begin(), files.end());

    std::vector<unicode> directories;

    for(auto it = files.begin(); it != files.end();) {
        unicode real_path = os::path::real_path(os::path::join(path, *it));
        if(os::path::is_dir(real_path)) {
            directories.push_back((*it));
            it = files.erase(it);
        } else {
            ++it;
        }
    }

    directories.insert(directories.end(), files.begin(), files.end());
    files = directories;

    if(os::path::is_dir(path) && !node) {
        auto image = Gtk::IconTheme::get_default()->load_icon("folder", Gtk::ICON_SIZE_MENU);

        node = &(*file_tree_store_->append());
        node->set_value(file_tree_columns_.name, Glib::ustring(os::path::split(path).second.encode()));
        node->set_value(file_tree_columns_.full_path, Glib::ustring(path.encode()));
        node->set_value(file_tree_columns_.image, image);
        node->set_value(file_tree_columns_.is_folder, true);
        node->set_value(file_tree_columns_.is_dummy, false);
    }

    for(auto f: files) {
        if(f == "." || f == "..") continue;

        if(f.starts_with(".")) {
            //Ignore hidden files and folders
            continue;
        }

        unicode full_name = os::path::join(path, f);

        bool ignore = false;
        for(auto glob: ignored_globs_) {
            if(glob::match(full_name, glob)) {
                //Ignore this file/folder if it matches the ignored globs
                ignore = true;
                break;
            }
        }

        if(ignore) {
            continue;
        }

        bool is_folder = os::path::is_dir(os::path::real_path(full_name));

        auto image = Gtk::IconTheme::get_default()->load_icon(
            (is_folder) ? "folder" : "document",
            Gtk::ICON_SIZE_MENU
        );


        const Gtk::TreeModel::Row* row;
        if(node) {
            row = &(*file_tree_store_->append(node->children()));
        } else {
            row = &(*file_tree_store_->append());
        }

        row->set_value(file_tree_columns_.name, Glib::ustring(f.encode()));
        row->set_value(file_tree_columns_.full_path, Glib::ustring(full_name.encode()));
        row->set_value(file_tree_columns_.image, image);
        row->set_value(file_tree_columns_.is_folder, is_folder);
        row->set_value(file_tree_columns_.is_dummy, false);

        if(is_folder) {
            //Add a dummy node under the folder
            const Gtk::TreeRow* dummy = &(*file_tree_store_->append(row->children()));
            dummy->set_value(file_tree_columns_.is_dummy, true);
        }
    }
}

void Window::rebuild_file_tree(const unicode& path) {
    file_tree_store_->clear();

    dirwalk(path, nullptr);

    window_file_tree_->expand_row(Gtk::TreeModel::Path("0"), false);
}

void Window::rebuild_open_list() {
    open_list_store_->clear();

    for(auto buffer: open_buffers_) {
        auto row = *(open_list_store_->append());

        row[open_list_columns_.name] = Glib::ustring(buffer->name().encode());
        row[open_list_columns_.buffer] = buffer;
    }
}


void Window::split() {
    assert(frames_.size() == 1);
    create_frame();
}

void Window::unsplit() {
    assert(frames_.size() == 2);
    frames_.pop_back(); //Destroy the second frame
}

void Window::on_buffer_modified(Buffer::ptr buffer) {
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
    on_buffer_modified(buffer);
}

void Window::new_buffer(const unicode& name) {
    Buffer::ptr buffer = std::make_shared<Buffer>(*this, name);

    //Watch for changes to the buffer
    buffer->signal_modified_changed().connect(
        sigc::bind(sigc::mem_fun(this, &Window::on_buffer_modified), buffer)
    );

    buffer->set_modified(true); //New file, mark as modified

    open_buffers_.push_back(buffer);

    activate_buffer(buffer);
    rebuild_open_list();
}

void Window::close_active_buffer() {
    close_buffer(frames_[current_frame_]->buffer());
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

void Window::close_buffer(const Glib::RefPtr<Gio::File>& file) {
    for(auto buffer: open_buffers_) {
        if(buffer->path() == file->get_path()) {
            close_buffer(buffer.get());
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
        sigc::bind(sigc::mem_fun(this, &Window::on_buffer_modified), buffer)
    );

    open_buffers_.push_back(buffer);

    activate_buffer(buffer);
    rebuild_open_list();
}

void Window::create_frame() {
    assert(frames_.size() < (uint32_t) FRAME_LIMIT);

    Frame::ptr frame = std::make_shared<Frame>(*this);

    //FIXME: Add frame to the container
    gtk_container_->add(frame->_gtk_box());
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
