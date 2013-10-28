#include "search.h"
#include "buffer.h"

namespace delimit {

const Glib::ustring SEARCH_HIGHLIGHT_TAG = "search_highlight";

void Search::set_buffer(Buffer *buffer)  {
    buffer_ = buffer;
    if(buffer_) {
        //Create the buffer tag if necessary
        if(!buffer_->_gtk_buffer()->get_tag_table()->lookup(SEARCH_HIGHLIGHT_TAG)) {
            auto tag = buffer_->_gtk_buffer()->create_tag (SEARCH_HIGHLIGHT_TAG);
            tag->property_background() = "#B3BF6F";
        }
    }
}

void Search::build_widgets() {
    find_next_button_.set_label("Find");
    find_next_button_.set_margin_left(5);

    pack_start(find_entry_, false, false);
    pack_start(find_next_button_, false, false);
    set_margin_bottom(5);

    find_entry_.signal_changed().connect(sigc::mem_fun(this, &Search::on_entry_changed));
}

void Search::on_entry_changed() {
    int highlighted = highlight_all(find_entry_.get_text().c_str());
    if(!highlighted) {
        //TODO: Turn entry red
    }
}

int Search::highlight_all(const unicode& string) {
    auto buffer = buffer_->_gtk_buffer();
    auto start = buffer->begin();
    auto end_of_file = buffer->end();
    auto end = start;

    bool case_sensitive = !(string.upper() == string || string.lower() == string);

    //Remove any existing highlights
    buffer->remove_tag_by_name(SEARCH_HIGHLIGHT_TAG, start, end_of_file);

    int highlighted = 0;
    while(start.forward_search(
              string.encode(), (case_sensitive) ? Gtk::TextSearchFlags(0) : Gtk::TEXT_SEARCH_CASE_INSENSITIVE,
              start, end)) {
        buffer->apply_tag_by_name(SEARCH_HIGHLIGHT_TAG, start, end);
        start = buffer->get_iter_at_offset(end.get_offset());
        ++highlighted;
    }
    return highlighted;
}

}
