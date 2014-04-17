#include <kazbase/logging.h>
#include <kazbase/unicode.h>
#include "frame.h"
#include "buffer.h"
#include "window.h"

namespace delimit {

enum IndentType {
    INDENT_TABS,
    INDENT_SPACES
};

std::pair<IndentType, int> detect_indentation(const unicode& text) {
    if(text.empty()) {
        return std::make_pair(INDENT_SPACES, 4);
    }

    std::vector<unicode> lines = text.split("\n");

    for(auto line: lines) {
        if(line.empty()) {
            continue;
        }

        std::string str = line.encode();
        auto iter = str.find_first_not_of(" \t\r\n"); //Find the first non-whitespace character

        unicode whitespace(std::string(str.begin(), str.begin() + iter)); //Convert back to unicode which is easier to work with

        if(whitespace.empty()) {
            continue; //Move onto the next line
        }

        if(whitespace.starts_with("\t")) {
            return std::make_pair(INDENT_TABS, 0); //Guess tab indentation
        } else {
            int i = 0;

            //Count the spaces
            while(i < whitespace.length() && whitespace[i] == ' ') {
                ++i;
            }

            //Return indent as spaces, then the number
            return std::make_pair(INDENT_SPACES, i);
        }
    }

    return std::make_pair(INDENT_SPACES, 4);
}

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
    source_view_.set_show_line_marks(true);
    source_view_.set_smart_home_end(Gsv::SMART_HOME_END_AFTER);

    search_.signal_close_requested().connect([&]() {
        parent_.buffer_search_button().set_active(false);
    });

    search_.set_no_show_all();

    auto mark_attrs = Gsv::MarkAttributes::create();
    Gdk::RGBA colour;
    colour.set_rgba(1.0, 0.5, 0, 0.15);
    mark_attrs->set_background(colour);
    source_view_.set_mark_attributes("coverage", mark_attrs, 0);

    set_search_visible(false);
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
        buffer_changed_connection_.disconnect();
    }

    buffer_ = buffer;

    source_view_.set_buffer(buffer_->_gtk_buffer());    
    signal_buffer_changed_(buffer_);

    auto manager = Gsv::StyleSchemeManager::get_default();
    source_view_.get_source_buffer()->set_style_scheme(manager->get_scheme("delimit"));

    buffer_->signal_loaded().connect([&](Buffer* buffer) {
          auto indent = detect_indentation(std::string(buffer->_gtk_buffer()->get_text()));

          if(indent.first == INDENT_TABS) {
              source_view_.set_insert_spaces_instead_of_tabs(false);
          } else {
              source_view_.set_indent_width(indent.second);
              source_view_.set_insert_spaces_instead_of_tabs(true);
          }
    });

    Glib::signal_idle().connect_once([&]() {
        scrolled_window_.get_vadjustment()->set_value(buffer_->retrieve_adjustment_value());

        auto indent = detect_indentation(std::string(buffer_->_gtk_buffer()->get_text()));

        if(indent.first == INDENT_TABS) {
            source_view_.set_insert_spaces_instead_of_tabs(false);
        } else {
            source_view_.set_indent_width(indent.second);
            source_view_.set_insert_spaces_instead_of_tabs(true);
        }
    });

    buffer_changed_connection_ = buffer_->_gtk_buffer()->signal_changed().connect(sigc::mem_fun(this, &Frame::check_undoable_actions));
    check_undoable_actions();
}

void Frame::check_undoable_actions() {
    this->parent_.set_undo_enabled(buffer_->_gtk_buffer()->can_undo());
    this->parent_.set_redo_enabled(buffer_->_gtk_buffer()->can_redo());
}


}
