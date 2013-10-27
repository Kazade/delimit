#ifndef DELIMIT_BUFFER_H
#define DELIMIT_BUFFER_H

#include <gtksourceviewmm.h>
#include <memory>

#include "base/os.h"
#include "utils/sigc_lambda.h"

namespace delimit {

class Window;

class Buffer {
public:
    typedef std::shared_ptr<Buffer> ptr;

    Buffer(Window& parent, const unicode& name):
        parent_(parent),
        name_(name) {

        gtk_buffer_ = Gsv::Buffer::create();
    }

    void _finish_read(Glib::RefPtr<Gio::FileInputStream> stream, Glib::RefPtr<Gio::AsyncResult> res) {
        auto bytes = stream->read_bytes_finish(res);
        gsize size = bytes->get_size();
        const char* str = (const char*)bytes->get_data(size);

        Glib::ustring result(str);

        Glib::signal_idle().connect([=]() -> bool {
            gtk_buffer_->set_text(result);
            return false;
        });
    }

    Buffer(Window& parent, const unicode& name, const Glib::RefPtr<Gio::File>& file):
        parent_(parent),
        name_(name),
        gio_file_(file) {

        is_saved_ = file->query_exists();

        if(is_saved_) {
            Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
            Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

            gtk_buffer_ = Gsv::Buffer::create(lang);

            auto stream = file->read();
            std::function<void (Glib::RefPtr<Gio::AsyncResult>)> func = std::bind(&Buffer::_finish_read, this, stream, std::placeholders::_1);
            stream->read_bytes_async(gsize(1000000), func);
        } else {
            gtk_buffer_ = Gsv::Buffer::create();
        }


    }

    Glib::RefPtr<Gsv::Buffer> _gtk_buffer() { return gtk_buffer_; }

    unicode name() const { return name_; }
    unicode path() const {
        if(gio_file_)
            return gio_file_->get_path();
        return "";
    }

private:        
    Window& parent_;
    unicode name_;
    Glib::RefPtr<Gio::File> gio_file_;

    Glib::RefPtr<Gsv::Buffer> gtk_buffer_;

    bool is_saved() const { return is_saved_; }

    bool is_saved_ = false;
};

}

#endif // BUFFER_H
