#ifndef DOCUMENT_VIEW_H
#define DOCUMENT_VIEW_H

#include <kazbase/unicode.h>
#include <kazbase/json/json.h>

#include <memory>
#include <gtkmm.h>
#include <gtksourceviewmm.h>

#include "coverage/coverage.h"
#include "linter/linter.h"

namespace delimit {

class Window;

typedef Glib::RefPtr<Gio::File> GioFilePtr;
typedef Glib::RefPtr<Gio::FileMonitor> GioFileMonitorPtr;

class DocumentView {
public:
    typedef std::shared_ptr<DocumentView> ptr;

    DocumentView(Window& window);
    DocumentView(Window& window, const unicode& filename);

    unicode name() const;
    unicode path() const;

    void save(const unicode& filename);
    void reload();
    void close();

    sigc::signal<void, GioFilePtr, GioFilePtr, Gio::FileMonitorEvent>& signal_file_changed() { return signal_file_changed_; }
    sigc::signal<void, DocumentView&>& signal_modified_changed() { return signal_modified_changed_; }
    sigc::signal<void, DocumentView&>& signal_closed() { return signal_closed_; }
    sigc::signal<void, DocumentView&>& signal_loaded() { return signal_loaded_; }

    unicode guess_mimetype() const;

    Glib::RefPtr<Gsv::Buffer> buffer() { return buffer_; }
    Gsv::View& view() { return view_; }
    Gtk::ScrolledWindow& container() { return scrolled_window_; }

    bool is_new_file() const;

    Window& window() { return window_; }

    void set_lint_errors(const ErrorList& errors) { lint_errors_ = errors; }
    ErrorList lint_errors() const { return lint_errors_; }

    void run_linters_and_stuff(bool force=false);

    bool completion_visible() const;
    void hide_completion();

    void scroll_to_line(int line);
private:
    Window& window_;

    // Annoyingly, the completion object doesn't have a get_visible function, only show/hide signals
    // so we need to maintain whether or not the completion window is visible ourselves
    bool completion_visible_ = false;

    sigc::signal<void, GioFilePtr, GioFilePtr, Gio::FileMonitorEvent> signal_file_changed_;
    sigc::signal<void, DocumentView&> signal_closed_;
    sigc::signal<void, DocumentView&> signal_modified_changed_;
    sigc::signal<void, DocumentView&> signal_loaded_;

    Gtk::ScrolledWindow scrolled_window_;
    Gsv::View view_;
    Glib::RefPtr<Gsv::Buffer> buffer_;

    GioFilePtr file_;
    GioFileMonitorPtr file_monitor_;
    std::string file_etag_;

    coverage::Coverage::ptr coverage_;
    linter::Linter::ptr linter_;

    ErrorList lint_errors_;

    void trim_trailing_newlines();
    void trim_trailing_whitespace();


    void create_buffer();
    void build_widgets();
    void connect_signals();
    void create_new_file();
    void open_file(const unicode& filename);
    void set_file(GioFilePtr file);
    void finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res);

    void connect_file_monitor();
    void disconnect_file_monitor();

    void handle_file_change_signal(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event);
    void handle_file_deleted();

    void file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event);

    void apply_language_to_buffer(const Glib::RefPtr<Gsv::Language>& language);
    void detect_and_apply_indentation();
    void apply_settings(const unicode& mimetype);
    std::pair<unicode, int> get_font_name_and_size_from_dconf();
    json::JSON current_settings_;

    void populate_popup(Gtk::Menu* menu);
};


}

#endif // DOCUMENT_VIEW_H
