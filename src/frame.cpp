#include "base/logging.h"
#include "base/unicode.h"
#include "frame.h"
#include "buffer.h"
#include "window.h"

namespace delimit {

void Frame::build_widgets() {
    scrolled_window_.add(source_view_);

    //file_chooser_box_.pack_start(file_chooser_, true, false, 0);
    //container_.pack_start(file_chooser_box_, false, false, 0);
    container_.pack_start(scrolled_window_, true, true, 0);
    container_.pack_start(search_, false, false, 0);


    auto settings = Gio::Settings::create("org.gnome.desktop.interface");
    auto font_name = settings->get_string("monospace-font-name");

    auto parts = unicode(font_name).split(" ");
    int size = 10;
    try {
        //Assume that the last part is the font size, if it's not
        //then fall back to size 10 and don't alter the font name
        size = parts.at(parts.size()-1).to_int();
        parts.pop_back();
        font_name = _u(" ").join(parts).encode();
    } catch(boost::bad_lexical_cast& e) {}

    Pango::FontDescription fdesc;
    fdesc.set_family(font_name);
    fdesc.set_size(size * PANGO_SCALE);
    source_view_.override_font(fdesc);

    source_view_.set_show_line_numbers(true);
    source_view_.set_tab_width(4);
    source_view_.set_auto_indent(true);
    source_view_.set_insert_spaces_instead_of_tabs(true);
    source_view_.set_draw_spaces(Gsv::DRAW_SPACES_SPACE | Gsv::DRAW_SPACES_TAB);
    source_view_.set_left_margin(4);
    source_view_.set_right_margin(4);

    search_.signal_close_requested().connect([&]() {
        parent_.buffer_search_button().set_active(false);
    });

    search_.set_no_show_all();

    //I have no idea why I need to do this in idle(), but it won't work on start up otherwise
    //At least this way we're thread safe!
    Glib::signal_idle().connect_once(sigc::bind(sigc::mem_fun(this, &Frame::set_search_visible), false));
}

void Frame::set_search_visible(bool value) {
    if(value) {
        search_.show();
        search_.show_all_children();
    } else {
        search_.hide();
    }
}

void Frame::set_buffer(Buffer *buffer) {
    if(buffer == buffer_) {
        return;
    }

    if(buffer_) {
        buffer_->store_adjustment_value(scrolled_window_.get_vadjustment()->get_value());
    }

    buffer_ = buffer;

    source_view_.set_buffer(buffer_->_gtk_buffer());    
    signal_buffer_changed_(buffer_);

    auto manager = Gsv::StyleSchemeManager::get_default();
    source_view_.get_source_buffer()->set_style_scheme(manager->get_scheme("delimit"));

    Glib::signal_idle().connect_once([&]() {
        scrolled_window_.get_vadjustment()->set_value(buffer_->retrieve_adjustment_value());
    });

    buffer_->signal_file_changed().connect(sigc::mem_fun(this, &Frame::file_changed_outside_editor));
    buffer_->_gtk_buffer()->signal_changed().connect(sigc::mem_fun(this, &Frame::check_undoable_actions));
    check_undoable_actions();
}

void Frame::check_undoable_actions() {
    this->parent_.set_undo_enabled(buffer_->_gtk_buffer()->can_undo());
    this->parent_.set_redo_enabled(buffer_->_gtk_buffer()->can_redo());
}

void Frame::file_changed_outside_editor(const Glib::RefPtr<Gio::File>& file,
    const Glib::RefPtr<Gio::File>& other_file, Gio::FileMonitorEvent event) {

    if(event == Gio::FILE_MONITOR_EVENT_CHANGED) {
        Gtk::MessageDialog dialog(parent_._gtk_window(), "File Changed", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

        dialog.set_message(_u("The file <i>{0}</i> has changed outside Delimit").format(os::path::split(file->get_path()).second).encode(), true);
        dialog.set_secondary_text("Do you want to reload?");
        dialog.add_button("Close", Gtk::RESPONSE_CLOSE);

        int response = dialog.run();

        switch(response) {
            case Gtk::RESPONSE_YES:
                buffer()->reload();
            break;
            case Gtk::RESPONSE_CLOSE:
                //Close
                parent_.close_buffer(file);
            break;
            case Gtk::RESPONSE_NO:
                return;
            break;
        }
    }
}

}
