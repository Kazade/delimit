#include "buffer.h"
#include "window.h"

#include "base/logging.h"

namespace delimit {

Buffer::Buffer(Window& parent, const unicode& name):
    parent_(parent),
    name_(name),
    adjustment_(0) {

    create_buffer();
    signal_file_changed().connect(sigc::mem_fun(this, &Buffer::on_file_changed));
}

Buffer::Buffer(Window& parent, const unicode& name, const Glib::RefPtr<Gio::File>& file):
    parent_(parent),
    name_(name),
    adjustment_(0) {

    set_gio_file(file);
    signal_file_changed().connect(sigc::mem_fun(this, &Buffer::on_file_changed));
}

void Buffer::create_buffer(Glib::RefPtr<Gsv::Language> lang) {
    if(!gtk_buffer_) {
        gtk_buffer_ = Gsv::Buffer::create(lang);
        gtk_buffer_->signal_changed().connect(sigc::mem_fun(this, &Buffer::on_buffer_changed));
        gtk_buffer_->signal_modified_changed().connect(sigc::mem_fun(this, &Buffer::on_signal_modified_changed));        ;
    }

    gtk_buffer_->set_language(lang);
}

Buffer::~Buffer() {
    L_DEBUG("Deleting buffer");
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

void Buffer::mark_as_new_file() {
    gio_file_.reset();
    if(gio_file_monitor_) {
        L_DEBUG("Wiping out file monitor");
        gio_file_monitor_->cancel();
        gio_file_monitor_.reset();
    }
    set_modified(true);
}

void Buffer::set_gio_file(const Glib::RefPtr<Gio::File>& file, bool reload) {
    if(gio_file_monitor_) {
        L_DEBUG("Disconnecting existing file monitor");
        gio_file_monitor_->cancel();
        gio_file_monitor_.reset();
    }

    gio_file_ = file;

    //This will detect the right language on save or load
    //and connect a file monitor
    if(gio_file_) {
        Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
        Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

        create_buffer(lang); //Create the buffer if necessary

        std::function<void (Glib::RefPtr<Gio::AsyncResult>)> func = std::bind(&Buffer::_finish_read, this, file, std::placeholders::_1);
        file->load_contents_async(func);
    } else {
        //Create the buffer without specifying a language
        create_buffer();
    }

    if(!gio_file_monitor_) {
        L_DEBUG("Connecting file monitor");
        gio_file_monitor_ = gio_file_->monitor_file();
        gio_file_monitor_->signal_changed().connect(sigc::mem_fun(this, &Buffer::file_changed));
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

        itr.forward_line(); //This maintains a newline at the end of the file, should probably make this optional

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

void Buffer::close() {
    if(modified()) {
        Gtk::MessageDialog dialog(parent_._gtk_window(), "Do you want to save your work?", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        dialog.set_message(_u("The file <i>{0}</i> has been changed since it was last saved.").format(name()).encode(), true);
        dialog.set_secondary_text("Do you want to save the file?");
        int response = dialog.run();
        switch(response) {
            case Gtk::RESPONSE_YES: {
                bool was_saved = parent_.toolbutton_save_clicked();
                if(!was_saved) {
                    L_DEBUG("User cancelled saving a file, so we won't close the buffer");
                    return;
                }
            }
            break;
            default:
                break;
        }
    }

    if(gio_file_monitor_) {
        gio_file_monitor_->cancel();
        gio_file_monitor_ = Glib::RefPtr<Gio::FileMonitor>();
    }
    signal_closed_(this);
}

void Buffer::save(const unicode& path) {
    L_DEBUG("Saving file: " + path.encode());
    trim_trailing_newlines();
    trim_trailing_whitespace();

    Glib::ustring text = _gtk_buffer()->get_text();
    _gtk_buffer()->set_modified(false);

    std::string new_etag;

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

void Buffer::on_file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event) {
    L_DEBUG(_u("Open file change event: {0}").format((int)event).encode());

    if(event == Gio::FILE_MONITOR_EVENT_DELETED || !os::path::exists(unicode(file->get_path()))) {
        Gtk::MessageDialog dialog(parent_._gtk_window(), "File Deleted", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        dialog.set_message(_u("The file <i>{0}</i> has been deleted").format(os::path::split(file->get_path()).second).encode(), true);
        dialog.set_secondary_text("Do you want to close this file?");
        int response = dialog.run();
        switch(response) {
            case Gtk::RESPONSE_YES:
                close();
            break;
            case Gtk::RESPONSE_NO:
                //If they didn't want to close the file, mark this one as a totally new file
                mark_as_new_file();
            break;
            default:
                return;
        }
    } else if(event == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
        Gtk::MessageDialog dialog(parent_._gtk_window(), "File Changed", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

        dialog.set_message(_u("The file <i>{0}</i> has changed outside Delimit").format(os::path::split(file->get_path()).second).encode(), true);
        dialog.set_secondary_text("Do you want to reload?");
        dialog.add_button("Close", Gtk::RESPONSE_CLOSE);

        int response = dialog.run();

        switch(response) {
            case Gtk::RESPONSE_YES:
                reload();
            break;
            case Gtk::RESPONSE_CLOSE:
                //Close
                close();
            break;
            case Gtk::RESPONSE_NO:
                //The user didn't want to reload, so mark the file as modified
                //so that they can see it doesn't match what's on disk
                set_modified(true);
            break;
            default:
                return;
        }
    }
}

void Buffer::on_buffer_changed() {
    _gtk_buffer()->set_modified(true);
}


}
