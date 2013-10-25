#ifndef DELIMIT_BUFFER_H
#define DELIMIT_BUFFER_H

#include <gtksourceviewmm.h>
#include <memory>

#include "base/os.h"

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

    Buffer(Window& parent, const unicode& name, const unicode& path):
        parent_(parent),
        name_(name),
        path_(path) {

        is_saved_ = os::path::exists(path);

        if(is_saved_) {
            Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
            Glib::RefPtr<Gsv::Language> lang = lm->guess_language(path.encode().c_str(), Glib::ustring());

            gtk_buffer_ = Gsv::Buffer::create(lang);
        } else {
            gtk_buffer_ = Gsv::Buffer::create();
        }
    }

    Glib::RefPtr<Gsv::Buffer> _gtk_buffer() { return gtk_buffer_; }

private:
    Glib::RefPtr<Gsv::Buffer> gtk_buffer_;

    Window& parent_;
    unicode name_;
    unicode path_;

    bool is_saved() const { return is_saved_; }

    bool is_saved_ = false;
};

}

#endif // BUFFER_H
