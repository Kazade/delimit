#include <kazbase/unicode.h>
#include "awesome_bar.h"
#include "window.h"
#include "buffer.h"

namespace delimit {

AwesomeBar::AwesomeBar(Window &parent):
    window_(parent) {

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

    entry_.set_width_chars(32);
    pack_start(entry_, false, false);

    entry_.signal_changed().connect([&]() {
        populate(entry_.get_text().c_str());
    });

    entry_.signal_activate().connect(sigc::mem_fun(this, &AwesomeBar::execute));

    Pango::FontDescription desc("sans-serif 26");
    entry_.override_font(desc);
}

void AwesomeBar::execute() {
    hide();
    entry_.set_text("");
}

void AwesomeBar::populate(const unicode &text) {
    if(text.starts_with(":") && text.length() > 1) {
        try {
            unicode number_text = text.lstrip(":");
            int line_number = number_text.to_int();

            auto buffer = window_.current_buffer()->buffer();
            auto iter = buffer->get_iter_at_line(line_number);
            window_.current_buffer()->view().scroll_to(iter, 0, 0.5, 0.5);
        } catch(std::exception& e) {
            return;
        }
    }
}

}
