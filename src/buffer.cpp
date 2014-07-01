#include "buffer.h"
#include "window.h"
#include "application.h"

#include <kazbase/logging.h>
#include <kazbase/fdo/base_directory.h>

namespace delimit {

Buffer::Buffer(Window& parent):
    parent_(parent),
    error_count_(0),
    adjustment_(0) {

    auto file = create_new_file(); //Create a new "unsaved" file
    create_buffer(); //Create the buffer
    set_gio_file(file); //Set the gio file
    signal_file_changed().connect(sigc::mem_fun(this, &Buffer::on_file_changed));

    set_modified(true); //This is an unsaved file really
}

Buffer::Buffer(Window& parent, const Glib::RefPtr<Gio::File>& file):
    parent_(parent),
    adjustment_(0),
    error_count_(0) {

    create_buffer();
    set_gio_file(file);    
    apply_language_to_buffer(guess_language_from_file(file));
    signal_file_changed().connect(sigc::mem_fun(this, &Buffer::on_file_changed));

    set_modified(false);
}

bool Buffer::is_new_file() const {
    return !os::path::exists(gio_file_->get_path());
}

Glib::RefPtr<Gio::File> Buffer::create_new_file() {
    /*
     *  Creates a new file. We create a Gio::File for a path in the tempfiles directory
     *  but don't actually create a file there. That way the lack of existence of the path
     *  allows us to determine whether the file is new, or has been deleted on disk
     */

    auto temp_files = tempfiles_directory();

    int existing_count = parent_.new_file_count();

    //FIXME: This will create multiple files with the same path if we have untitled
    //files in two windows. I'm not sure if this is a problem or not considering these
    //paths are just placeholders. In future it may be!
    auto untitled_filename = _u("Untitled {0}").format(existing_count + 1);
    auto full_path = os::path::join(temp_files, untitled_filename);
    auto file = Gio::File::create_for_path(full_path.encode());
    return file;
}

void Buffer::apply_language_to_buffer(const Glib::RefPtr<Gsv::Language>& language) {
    gtk_buffer_->set_language(language);
}

void Buffer::create_buffer() {
    if(!gtk_buffer_) {
        gtk_buffer_ = Gsv::Buffer::create();
        gtk_buffer_->signal_changed().connect(sigc::mem_fun(this, &Buffer::on_buffer_changed));
        gtk_buffer_->signal_modified_changed().connect(sigc::mem_fun(this, &Buffer::on_signal_modified_changed));
    }
}

void Buffer::on_signal_modified_changed() {
    signal_modified_changed_(this);
}

Buffer::~Buffer() {
    L_DEBUG("Deleting buffer");
}

void Buffer::run_linters_and_stuff() {
    //Check for coverage stats
    auto language = gtk_buffer_->get_language();
    unicode name = (language) ? unicode(language->get_name()) : "";
    if(name == "Python") {
        coverage_ = std::make_shared<coverage::PythonCoverage>();
        coverage_->apply_to_buffer(this);

        linter_ = std::make_shared<linter::PythonLinter>();
        linter_->apply_to_buffer(this);

    } else {
        if(coverage_) {
           coverage_->clear_buffer(this);
           coverage_.reset();
        }

        if(linter_) {
           linter_->clear_buffer(this);
           linter_.reset();
        }
    }
}

void Buffer::_finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res) {
    char* output;
    gsize size;

    file->load_contents_finish(res, output, size);

    Glib::ustring result(output);

    gtk_buffer_->begin_not_undoable_action();
    gtk_buffer_->set_text(result);
    gtk_buffer_->set_modified(false);
    gtk_buffer_->end_not_undoable_action();

    signal_loaded_(this);

    Glib::signal_idle().connect_once(sigc::bind(&Buffer::run_linters_and_stuff, this));
}

Glib::RefPtr<Gsv::Buffer> Buffer::_gtk_buffer() {
    return gtk_buffer_;
}

unicode Buffer::name() const {
    return os::path::split(gio_file_->get_path()).second;
}

unicode Buffer::path() const {
    assert(gio_file_);
    return gio_file_->get_path();
}

bool Buffer::modified() const {
    return gtk_buffer_->get_modified();
}

Glib::RefPtr<Gsv::Language> Buffer::guess_language_from_file(const GioFilePtr& file) {
    Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
    Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

    return lang;
}

void Buffer::set_gio_file(const Glib::RefPtr<Gio::File>& file, bool reload) {
    assert(gtk_buffer_);

    if(gio_file_monitor_) {
        L_DEBUG("Disconnecting existing file monitor");
        gio_file_monitor_->cancel();
        gio_file_monitor_.reset();
    }

    gio_file_ = file;

    //This will connect a file monitor
    if(gio_file_ && !is_new_file()) {
        file_etag_ = gio_file_->query_info(G_FILE_ATTRIBUTE_ETAG_VALUE)->get_etag();
        L_DEBUG("File etag: " + file_etag_);

        if(reload) {
            std::function<void (Glib::RefPtr<Gio::AsyncResult>)> func = std::bind(&Buffer::_finish_read, this, file, std::placeholders::_1);
            file->load_contents_async(func);
        }

        if(!gio_file_monitor_) {
            L_DEBUG("Connecting file monitor");
            gio_file_monitor_ = gio_file_->monitor_file();
            gio_file_monitor_->signal_changed().connect(sigc::mem_fun(this, &Buffer::file_changed));
        }
    } else {
        file_etag_ = ""; //Wipe out the etag
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

    if(gio_file_->get_path() == path.encode()) {
        //FIXME: Use entity tag arguments to make sure that the file
        //didn't change since the last time we saved
        gio_file_->replace_contents(std::string(text.c_str()), "", file_etag_);
        set_gio_file(gio_file_, false); //Don't reload the file, reconnect the monitor
    } else {
        auto file = Gio::File::create_for_path(path.encode());
        if(!os::path::exists(path)) {
            file->create_file();
        }
        file->replace_contents(text, "", file_etag_);
        set_gio_file(file, false); //Don't reload the file, reconnect the monitor
    }

    set_modified(false);

    parent_.rebuild_open_list();
    run_linters_and_stuff();
}

void Buffer::on_file_deleted(const unicode& filename) {
    L_DEBUG("Processing deleted file");

    if(!os::path::exists(filename)) {
        Gtk::MessageDialog dialog(parent_._gtk_window(), "File Deleted", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        dialog.set_message(_u("The file <i>{0}</i> has been deleted").format(os::path::split(filename).second).encode(), true);
        dialog.set_secondary_text("Do you want to close this file?");
        int response = dialog.run();
        switch(response) {
            case Gtk::RESPONSE_YES:
                close();
            break;
            case Gtk::RESPONSE_NO:
                //If they didn't want to close the file, mark this file as modified from on disk
                set_modified(true);
            break;
            default:
                return;
        }
    }
}

void Buffer::on_file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event) {
    unicode event_type;
    switch((int)event) {
        case 0: event_type = "CHANGED"; break;
        case 1: event_type = "CHANGES_DONE_HINT"; break;
        case 2: event_type = "DELETED"; break;
        case 3: event_type = "CREATED"; break;
        case 4: event_type = "ATTRIBUTE_CHANGED"; break;
        case 5: event_type = "PRE_UNMOUNT"; break;
        case 6: event_type = "UNMOUNTED"; break;
        case 7: event_type = "MOVED"; break;
        default: event_type = "UNKNOWN";
    }

    L_DEBUG(_u("Open file change event: {0}").format(event_type).encode());

    if(event == Gio::FILE_MONITOR_EVENT_DELETED) {
        Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(this, &Buffer::on_file_deleted), file->get_path()));
    }  else if(event == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
        std::string etag = file->query_info(G_FILE_ATTRIBUTE_ETAG_VALUE)->get_etag();

        //Ignore the event if the etag matches one that we've saved
        if(etag != file_etag_) {
            L_DEBUG("etag doesn't match, so prompting:" + etag + " <> " + file_etag_);

            Gtk::MessageDialog dialog(parent_._gtk_window(), "File Changed", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

            dialog.set_message(_u("The file <i>{0}</i> has changed outside Delimit").format(os::path::split(file->get_path()).second).encode(), true);
            dialog.set_secondary_text("Do you want to reload?");
            dialog.add_button("Close", Gtk::RESPONSE_CLOSE);

            //Only display the dialog if we were modified otherwise default to
            //reloading automatically
            int response = Gtk::RESPONSE_YES;
            if(this->modified()) {
                response = dialog.run();
            }

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
}

void Buffer::on_buffer_changed() {
    _gtk_buffer()->set_modified(true);
}


}
