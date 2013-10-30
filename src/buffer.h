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

    Buffer(Window& parent, const unicode& name);
    Buffer(Window& parent, const unicode& name, const Glib::RefPtr<Gio::File>& file);

    unicode name() const;
    unicode path() const;

    bool modified() const;
    void set_modified(bool value) { gtk_buffer_->set_modified(value); }

    void save(const unicode& path);

    void _finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res);
    Glib::RefPtr<Gsv::Buffer> _gtk_buffer();

    Glib::SignalProxy0<void> signal_modified_changed() { return _gtk_buffer()->signal_modified_changed(); }
private:        
    Window& parent_;
    unicode name_;
    Glib::RefPtr<Gio::File> gio_file_;

    Glib::RefPtr<Gsv::Buffer> gtk_buffer_;

    bool is_saved() const { return bool(gio_file_); }

};

}

#endif // BUFFER_H
