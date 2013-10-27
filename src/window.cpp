#include <gtkmm.h>

#include "window.h"

#include "base/exceptions.h"
#include "base/unicode.h"
#include "base/fdo/base_directory.h"
#include "base/logging.h"

namespace delimit {

const int FRAME_LIMIT = 2;
const unicode UI_FILE = "delimit/ui/delimit.glade";

Window::Window() {
    L_DEBUG("Creating window with empty buffer");

    file_tree_store_ = Gtk::TreeStore::create(file_tree_columns_);
    open_list_store_ = Gtk::ListStore::create(open_list_columns_);

    build_widgets();
    new_buffer("Untitled");
}

Window::Window(const std::vector<Glib::RefPtr<Gio::File>>& files) {
    file_tree_store_ = Gtk::TreeStore::create(file_tree_columns_);
    open_list_store_ = Gtk::ListStore::create(open_list_columns_);

    build_widgets();

    if(files.size() == 1 && files[0]->query_file_type() == Gio::FILE_TYPE_DIRECTORY) {
        type_ = WINDOW_TYPE_FOLDER;

        rebuild_file_tree(files[0]->get_path());
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

    builder->get_widget("open_file_list", open_file_list_);
    open_file_list_->set_model(open_list_store_);
    open_file_list_->append_column("Name", open_list_columns_.name);
    open_file_list_->signal_row_activated().connect(sigc::mem_fun(this, &Window::on_list_signal_row_activated));

    builder->get_widget("buffer_undo", buffer_undo_);
    builder->get_widget("buffer_undo", buffer_redo_);

    assert(gtk_window_);

    create_frame(); //Create the default frame
}

void Window::on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    auto row = *(file_tree_store_->get_iter(path));

    if(!row[file_tree_columns_.is_folder]) {
        Glib::ustring full_path = row[file_tree_columns_.full_path];
        open_buffer(Gio::File::create_for_path(full_path));
    }
}

void Window::on_list_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    auto row = *(open_list_store_->get_iter(path));

    Buffer::ptr buffer = row[open_list_columns_.buffer];
    frames_[current_frame_]->set_buffer(buffer.get());
}

void Window::dirwalk(const unicode& path, const Gtk::TreeRow* node) {
    auto files = os::path::list_dir(path);
    std::sort(files.begin(), files.end());

    std::vector<unicode> directories;

    for(auto it = files.begin(); it != files.end();) {
        if(os::path::is_dir(os::path::join(path, *it))) {
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
    }

    for(auto f: files) {
        if(f == "." || f == "..") continue;

        unicode full_name = os::path::join(path, f);

        bool is_folder = os::path::is_dir(full_name);

        auto image = Gtk::IconTheme::get_default()->load_icon(
            (is_folder) ? "folder" : "document",
            Gtk::ICON_SIZE_MENU
        );


        const Gtk::TreeModel::Row* row;
        if(node) {
            L_DEBUG(_u("Appending child node for {0}").format(f));
            row = &(*file_tree_store_->append(node->children()));
        } else {
            L_DEBUG(_u("Appending node for {0}").format(f));
            row = &(*file_tree_store_->append());
        }

        row->set_value(file_tree_columns_.name, Glib::ustring(f.encode()));
        row->set_value(file_tree_columns_.full_path, Glib::ustring(full_name.encode()));
        row->set_value(file_tree_columns_.image, image);
        row->set_value(file_tree_columns_.is_folder, is_folder);

        if(is_folder) {
            dirwalk(full_name, row);
        }
    }
}

void Window::rebuild_file_tree(const unicode& path) {
    file_tree_store_->clear();

    dirwalk(path, nullptr);
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

void Window::new_buffer(const unicode& name) {
    Buffer::ptr buffer = std::make_shared<Buffer>(*this, name);

    open_buffers_.push_back(buffer);

    frames_[current_frame_]->set_buffer(buffer.get());

    rebuild_open_list();
}

void Window::open_buffer(const Glib::RefPtr<Gio::File> &file) {
    assert(current_frame_ >= 0 && current_frame_ < (int32_t) frames_.size());

    if(!file->query_exists()) {
        throw IOError(_u("Path does not exist: {0}").format(file->get_path()));
    }

    for(auto buffer: open_buffers_) {
        if(buffer->path() == file->get_path()) {
            //Just activate, don't open!
            frames_[current_frame_]->set_buffer(buffer.get());
            return;
        }
    }

    unicode name = os::path::split(file->get_path()).second;

    Buffer::ptr buffer = std::make_shared<Buffer>(*this, name, file);
    open_buffers_.push_back(buffer);

    frames_[current_frame_]->set_buffer(buffer.get());

    rebuild_open_list();
}

void Window::create_frame() {
    assert(frames_.size() < FRAME_LIMIT);

    Frame::ptr frame = std::make_shared<Frame>(*this);

    //FIXME: Add frame to the container
    gtk_container_->add(frame->_gtk_box());
    frame->_gtk_box().show_all();

    frames_.push_back(frame);
}

}
