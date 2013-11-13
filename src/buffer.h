#ifndef DELIMIT_BUFFER_H
#define DELIMIT_BUFFER_H

#include <gtksourceviewmm.h>
#include <memory>

#include "base/os.h"
#include "utils/sigc_lambda.h"

namespace delimit {

class Window;

typedef Glib::RefPtr<Gio::File> GioFilePtr;

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

    sigc::signal<void, GioFilePtr, GioFilePtr, Gio::FileMonitorEvent>& signal_file_changed() { return signal_file_changed_; }

    void reload() {
        gio_file_monitor_.reset();
        set_gio_file(gio_file_, true);
    }

private:
    void set_gio_file(const GioFilePtr& file, bool reload=true);
    void file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event) {
        signal_file_changed_(file, other_file, event);
    }

    sigc::signal<void, GioFilePtr, GioFilePtr, Gio::FileMonitorEvent> signal_file_changed_;

    Window& parent_;
    unicode name_;
    Glib::RefPtr<Gio::File> gio_file_;
    Glib::RefPtr<Gio::FileMonitor> gio_file_monitor_;

    Glib::RefPtr<Gsv::Buffer> gtk_buffer_;    

    bool is_saved() const { return bool(gio_file_); }

    void on_buffer_changed();
};

}

#endif // BUFFER_H
