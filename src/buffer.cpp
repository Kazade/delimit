#include "buffer.h"
#include "window.h"

namespace delimit {

Buffer::Buffer(Window& parent, const unicode& name):
    parent_(parent),
    name_(name),
    adjustment_(0) {

    gtk_buffer_ = Gsv::Buffer::create();
    gtk_buffer_->signal_changed().connect(sigc::mem_fun(this, &Buffer::on_buffer_changed));
}

Buffer::Buffer(Window& parent, const unicode& name, const Glib::RefPtr<Gio::File>& file):
    parent_(parent),
    name_(name),
    adjustment_(0) {

    set_gio_file(file);

    gtk_buffer_->signal_changed().connect(sigc::mem_fun(this, &Buffer::on_buffer_changed));
}

void Buffer::_finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res) {
    char* output;
    gsize size;

    file->load_contents_finish(res, output, size);

    Glib::ustring result(output);

    Glib::signal_idle().connect([=]() -> bool {
        gtk_buffer_->begin_not_undoable_action();
        gtk_buffer_->set_text(result);
        gtk_buffer_->set_modified(false);
        gtk_buffer_->end_not_undoable_action();
        return false;
    });
}

Glib::RefPtr<Gsv::Buffer> Buffer::_gtk_buffer() {
    return gtk_buffer_;
}

unicode Buffer::name() const {
    return name_;
}

unicode Buffer::path() const {
    if(gio_file_)
        return gio_file_->get_path();
    return "";
}

bool Buffer::modified() const {
    return gtk_buffer_->get_modified();
}

void Buffer::set_gio_file(const Glib::RefPtr<Gio::File>& file, bool reload) {
    gio_file_ = file;

    //This will detect the right language on save or load
    //and connect a file monitor
    if(gio_file_) {
        if(!gio_file_monitor_) {
            gio_file_monitor_ = gio_file_->monitor_file();
            gio_file_monitor_->signal_changed().connect(sigc::mem_fun(this, &Buffer::file_changed));
        }

        Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
        Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

        if(reload) {
            gtk_buffer_ = Gsv::Buffer::create(lang);

            std::function<void (Glib::RefPtr<Gio::AsyncResult>)> func = std::bind(&Buffer::_finish_read, this, file, std::placeholders::_1);
            file->load_contents_async(func);
        }
    } else {
        if(reload) {
            gtk_buffer_ = Gsv::Buffer::create();
        }
    }
}

void Buffer::trim_trailing_newlines() {
    //FIXME: Don't strip whitespace if the language doesn't like it (e.g. 'diff')
    auto buffer_end = _gtk_buffer()->end();
    if(buffer_end.starts_line()) {
        auto itr = buffer_end;
        while(itr.backward_line()) {
            if(!itr.ends_line()) {
                itr.forward_to_line_end();
                break;
            }
        }

        _gtk_buffer()->erase(itr, buffer_end);
    }
}

bool is_whitespace(Gtk::TextIter& iter) {
    for(auto ch: std::string("\t\v\f ")) {
        if(iter.get_char() == (gunichar) ch) {
            return true;
        }
    }

    return false;
}

void Buffer::trim_trailing_whitespace() {
    //FIXME: Don't strip whitespace if the language doesn't like it (e.g. 'diff')
    auto current = _gtk_buffer()->begin();

    while(current != _gtk_buffer()->end()) {
        current.forward_to_line_end(); //Move to the newline character
        auto line_end = current; //Store the end of the line
        while(is_whitespace(--current)) {} //Move backward until we get to non-whitespace

        current = _gtk_buffer()->erase(++current, line_end);

        current.forward_line(); //Move to the next line, or the end of the final line
    }
}

void Buffer::save(const unicode& path) {
    trim_trailing_newlines();
    trim_trailing_whitespace();

    Glib::ustring text = _gtk_buffer()->get_text();
    _gtk_buffer()->set_modified(false);

    std::string new_etag;

    if(gio_file_ || gio_file_monitor_) {
        //We're saving so wipe out the monitor until we're done
        gio_file_monitor_ = Glib::RefPtr<Gio::FileMonitor>();
    }

    //If we are saving the buffer for the first time over an existing path
    //then create the gio_file_ so we can replace the contents below
    if(!gio_file_ && os::path::exists(path)) {
        gio_file_ = Gio::File::create_for_path(path.encode());
    }

    if(gio_file_ && gio_file_->get_path() == path.encode()) {
        //FIXME: Use entity tag arguments to make sure that the file
        //didn't change since the last time we saved
        gio_file_->replace_contents(std::string(text.c_str()), "", new_etag);
        set_gio_file(gio_file_, false); //Don't reload the file, reconnect the monitor
    } else {
        auto file = Gio::File::create_for_path(path.encode());
        file->create_file();
        file->replace_contents(text, "", new_etag);
        set_gio_file(file, false); //Don't reload the file, reconnect the monitor
    }

    set_name(os::path::split(gio_file_->get_path()).second);
    parent_.rebuild_open_list();
}

void Buffer::on_buffer_changed() {
    _gtk_buffer()->set_modified(true);
}


}
