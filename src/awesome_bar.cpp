#include <kazbase/unicode.h>
#include "awesome_bar.h"
#include "frame.h"
#include "buffer.h"

namespace delimit {

AwesomeBar::AwesomeBar(Frame *parent):
    frame_(parent) {

    build_widgets();
}

void AwesomeBar::build_widgets() {
    set_margin_left(20);
    set_margin_right(20);
    set_margin_top(20);
    set_margin_bottom(20);
    set_no_show_all();
    set_valign(Gtk::ALIGN_START);
    set_halign(Gtk::ALIGN_CENTER);

    entry_.set_width_chars(100);
    pack_start(entry_, false, false);

    entry_.signal_changed().connect([&]() {
        populate(entry_.get_text().c_str());
    });
}

void AwesomeBar::populate(const unicode &text) {
    if(text.starts_with(":") && text.length() > 1) {
        try {
            unicode number_text = text.lstrip(":");
            int line_number = number_text.to_int();

            auto buffer = frame_->buffer()->_gtk_buffer();
            auto iter = buffer->get_iter_at_line(line_number);
            frame_->view().scroll_to(iter, 0, 0.5, 0.5);
        } catch(boost::bad_lexical_cast& e) {
            return;
        }
    }
}

}
