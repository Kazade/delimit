#include "buffer.h"

namespace delimit {

Buffer::Buffer(Window& parent, const unicode& name):
    parent_(parent),
    name_(name) {

    gtk_buffer_ = Gsv::Buffer::create();
    gtk_buffer_->signal_changed().connect(sigc::mem_fun(this, &Buffer::on_buffer_changed));
}

Buffer::Buffer(Window& parent, const unicode& name, const Glib::RefPtr<Gio::File>& file):
    parent_(parent),
    name_(name),
    gio_file_(file) {

    if(gio_file_) {
        Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
        Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

        gtk_buffer_ = Gsv::Buffer::create(lang);

        std::function<void (Glib::RefPtr<Gio::AsyncResult>)> func = std::bind(&Buffer::_finish_read, this, file, std::placeholders::_1);
        file->load_contents_async(func);
    } else {
        gtk_buffer_ = Gsv::Buffer::create();
    }
    gtk_buffer_->signal_changed().connect(sigc::mem_fun(this, &Buffer::on_buffer_changed));
}

void Buffer::_finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res) {
    char* output;
    gsize size;

    file->load_contents_finish(res, output, size);

    Glib::ustring result(output);

    Glib::signal_idle().connect([=]() -> bool {
        gtk_buffer_->set_text(result);
        gtk_buffer_->set_modified(false);
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

void Buffer::save(const unicode& path) {
    Glib::ustring text = _gtk_buffer()->get_text();
    _gtk_buffer()->set_modified(false);

    std::string new_etag;
    if(gio_file_) {
        //FIXME: Use entity tag arguments to make sure that the file
        //didn't change since the last time we saved
        gio_file_->replace_contents(std::string(text.c_str()), "", new_etag);
    } else {
        gio_file_ = Gio::File::create_for_path(path.encode());
        gio_file_->create_file();
        gio_file_->replace_contents(text, "", new_etag);
    }        
}

void Buffer::on_buffer_changed() {
    _gtk_buffer()->set_modified(true);
}


}
