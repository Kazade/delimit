#include "buffer.h"

namespace delimit {

Buffer::Buffer(Window& parent, const unicode& name):
    parent_(parent),
    name_(name) {

    gtk_buffer_ = Gsv::Buffer::create();
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


}

void Buffer::_finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res) {
    char* output;
    gsize size;

    file->load_contents_finish(res, output, size);

    Glib::ustring result(output);

    Glib::signal_idle().connect([=]() -> bool {
        gtk_buffer_->set_text(result);
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
/*
    auto start_iter = _gtk_buffer()->get_start_iter();
    auto end_iter = _gtk_buffer()->get_end_iter();

    Glib::ustring text = gtk_buffer()->get_text(start_iter, end_iter, false);
    _gtk_buffer()->set_modified(false);

    if(gio_file_) {
        gio_file_->replace_contents(text);
    } else {
        gio_file_ = Gio::File::create_for_path(path);
        gio_file_->replace_contents(text);
    }*/
}



}
