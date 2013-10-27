#include "frame.h"
#include "buffer.h"

namespace delimit {

void Frame::build_widgets() {

    scrolled_window_.add(source_view_);
    container_.pack_start(scrolled_window_, true, true);

    Pango::FontDescription fdesc;
    fdesc.set_family("monospace");
    source_view_.override_font(fdesc);
}

void Frame::set_buffer(Buffer *buffer) {
    buffer_ = buffer;

    source_view_.set_buffer(buffer_->_gtk_buffer());
}

}
