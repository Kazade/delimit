#include "base/logging.h"

#include "frame.h"
#include "buffer.h"

namespace delimit {

void Frame::build_widgets() {    
    scrolled_window_.add(source_view_);

    //file_chooser_box_.pack_start(file_chooser_, true, false, 0);
    //container_.pack_start(file_chooser_box_, false, false, 0);
    container_.pack_start(scrolled_window_, true, true, 0);

    Pango::FontDescription fdesc;
    fdesc.set_family("monospace");
    source_view_.override_font(fdesc);

    source_view_.set_show_line_numbers(true);
    source_view_.set_tab_width(4);
    source_view_.set_auto_indent(true);
    source_view_.set_insert_spaces_instead_of_tabs(true);
    source_view_.set_draw_spaces(Gsv::DRAW_SPACES_SPACE | Gsv::DRAW_SPACES_TAB);
}

void Frame::set_buffer(Buffer *buffer) {
    buffer_ = buffer;

    source_view_.set_buffer(buffer_->_gtk_buffer());

    auto manager = Gsv::StyleSchemeManager::get_default();
    source_view_.get_source_buffer()->set_style_scheme(manager->get_scheme("delimit"));
}

}
