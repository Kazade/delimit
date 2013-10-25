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

    build_widgets();
    new_buffer("Untitled");
}

Window::Window(const std::vector<Glib::RefPtr<Gio::File>>& files) {
    build_widgets();

    if(files.size() == 1 && files[0]->query_file_type() == Gio::FILE_TYPE_DIRECTORY) {
        type_ = WINDOW_TYPE_FOLDER;
    } else {
        type_ = WINDOW_TYPE_FILE;

        for(auto file: files) {
            open_buffer(file);
        }
    }
}

void Window::build_widgets() {
    std::string ui_file = fdo::xdg::find_data_file(UI_FILE).encode();
    auto builder = Gtk::Builder::create_from_file(ui_file);
    builder->get_widget("main_window", gtk_window_);
    builder->get_widget("window_container", gtk_container_);
    builder->get_widget("window_file_tree", window_file_tree_);

    builder->get_widget("buffer_undo", buffer_undo_);
    builder->get_widget("buffer_undo", buffer_redo_);

    assert(gtk_window_);

    create_frame(); //Create the default frame

    if(type_ == WINDOW_TYPE_FILE) {
        //Don't show the folder tree on FILE windows
        window_file_tree_->hide();

        //FIXME: Replace with an open file listing instead
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
}

void Window::open_buffer(const Glib::RefPtr<Gio::File> &file) {
    assert(current_frame_ >= 0 && current_frame_ < (int32_t) frames_.size());

    if(!file->query_exists()) {
        throw IOError(_u("Path does not exist: {0}").format(file->get_path()));
    }

    unicode name = os::path::split(file->get_path()).second;

    Buffer::ptr buffer = std::make_shared<Buffer>(*this, name, file);
    open_buffers_.push_back(buffer);

    frames_[current_frame_]->set_buffer(buffer.get());
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
