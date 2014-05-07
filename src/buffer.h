#ifndef DELIMIT_BUFFER_H
#define DELIMIT_BUFFER_H

#include <gtksourceviewmm.h>
#include <memory>

#include <kazbase/os.h>
#include "utils/sigc_lambda.h"
#include "coverage/coverage.h"
#include "linter/linter.h"

namespace delimit {

class Window;

typedef Glib::RefPtr<Gio::File> GioFilePtr;

class Buffer {
public:
    typedef std::shared_ptr<Buffer> ptr;

    Buffer(Window& parent, const unicode& name);
    Buffer(Window& parent, const unicode& name, const Glib::RefPtr<Gio::File>& file);
    ~Buffer();

    unicode name() const;
    void set_name(const unicode& name) {
        name_ = name;
    }

    unicode path() const;

    bool modified() const;
    void set_modified(bool value) { gtk_buffer_->set_modified(value); }

    void save(const unicode& path);
    void close();

    void _finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res);
    Glib::RefPtr<Gsv::Buffer> _gtk_buffer();

    sigc::signal<void, Buffer*>& signal_modified_changed() { return signal_modified_changed_; }
    sigc::signal<void, Buffer*>& signal_closed() { return signal_closed_; }
    sigc::signal<void, GioFilePtr, GioFilePtr, Gio::FileMonitorEvent>& signal_file_changed() { return signal_file_changed_; }
    sigc::signal<void, Buffer*>& signal_loaded() { return signal_loaded_; }

    void reload() {
        set_gio_file(gio_file_, true);
    }

    void store_adjustment_value(double adjust) {
        adjustment_ = adjust;
    }
    double retrieve_adjustment_value() const { return adjustment_; }

    const Window& window() const { return parent_; }
    Window& window() { return parent_; }

    void mark_as_new_file();
    void set_error_count(int32_t count) { error_count_ = count; }
    int32_t error_count() const { return error_count_; }
private:
    int32_t error_count_;

    void run_linters_and_stuff();

    void create_buffer(Glib::RefPtr<Gsv::Language> lang=Glib::RefPtr<Gsv::Language>());

    void trim_trailing_newlines();
    void trim_trailing_whitespace();

    void set_gio_file(const GioFilePtr& file, bool reload=true);

    void on_file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event);
    void on_file_deleted(const unicode& filename);

    void file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event) {
        signal_file_changed_(file, other_file, event);
    }

    void on_signal_modified_changed() {
        signal_modified_changed_(this);
    }

    sigc::signal<void, GioFilePtr, GioFilePtr, Gio::FileMonitorEvent> signal_file_changed_;
    sigc::signal<void, Buffer*> signal_closed_;
    sigc::signal<void, Buffer*> signal_modified_changed_;
    sigc::signal<void, Buffer*> signal_loaded_;

    Window& parent_;
    unicode name_;
    Glib::RefPtr<Gio::File> gio_file_;
    Glib::RefPtr<Gio::FileMonitor> gio_file_monitor_;
    std::string file_etag_;

    Glib::RefPtr<Gsv::Buffer> gtk_buffer_;    

    double adjustment_;

    bool is_saved() const { return bool(gio_file_); }

    void on_buffer_changed();

    coverage::Coverage::ptr coverage_;
    linter::Linter::ptr linter_;
};

}

#endif // BUFFER_H
