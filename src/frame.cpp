#include "frame.h"
#include "buffer.h"

namespace delimit {

void Frame::build_widgets() {
    container_.pack_start(source_view_, true, true);
}

void Frame::set_buffer(Buffer *buffer) {
    buffer_ = buffer;

    source_view_.set_buffer(buffer_->_gtk_buffer());
}

}
