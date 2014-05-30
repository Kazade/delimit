#include "search.h"
#include "buffer.h"
#include "frame.h"

namespace delimit {

const Glib::ustring SEARCH_HIGHLIGHT_TAG = "search_highlight";

Search::Search(Frame* parent):
    frame_(parent) {

    build_widgets();
}

void Search::_connect_signals() {
    assert(frame_);
    frame_->signal_buffer_changed().connect(sigc::mem_fun(this, &Search::on_buffer_changed));
    close_button_.signal_clicked().connect(sigc::mem_fun(this, &Search::hide));
    replace_button_.signal_clicked().connect(sigc::mem_fun(this, &Search::on_replace_clicked));
    replace_all_button_.signal_clicked().connect(sigc::mem_fun(this, &Search::on_replace_all_clicked));
}

void Search::show() {
    Gtk::Box::show();
    find_entry_.grab_focus();

    toggle_replace(false);

    //If there is some text selected in the buffer, use that as the default text
    Gtk::TextIter start, end;
    if(frame_->buffer()->_gtk_buffer()->get_selection_bounds(start, end)) {
        find_entry_.set_text(frame_->buffer()->_gtk_buffer()->get_slice(start, end));
        find_entry_.select_region(0, -1);
    }
}

Buffer* Search::buffer() {
    return frame_->buffer();
}

void Search::on_buffer_changed(Buffer *buffer)  {
    if(buffer) {
        //Create the buffer tag if necessary
        if(!buffer->_gtk_buffer()->get_tag_table()->lookup(SEARCH_HIGHLIGHT_TAG)) {
            auto tag = buffer->_gtk_buffer()->create_tag (SEARCH_HIGHLIGHT_TAG);
            tag->property_background() = "#1EBCFF";
        }
    }
}

void Search::build_widgets() {
    find_next_button_.set_label("Find");
    find_next_button_.set_margin_left(5);

    replace_button_.set_label("Replace");
    replace_button_.set_margin_left(5);

    replace_all_button_.set_label("Replace all");
    replace_all_button_.set_margin_left(5);

    case_sensitive_.set_label("Match case?");
    case_sensitive_.set_margin_right(5);

    Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::IconTheme::get_default()->load_icon("gtk-close", Gtk::ICON_SIZE_MENU)));
    close_button_.set_image(*image);
    close_button_.set_relief(Gtk::RELIEF_NONE);
    close_button_.set_border_width(0);
    close_button_.set_focus_on_click(false);
    close_button_.set_margin_right(5);

    replace_entry_.set_margin_left(5);

    pack_start(find_entry_, false, false);
    pack_start(find_next_button_, false, false);

    pack_start(replace_entry_, false, false);
    pack_start(replace_button_, false, false);
    pack_start(replace_all_button_, false, false);
    pack_end(close_button_, false, false);
    pack_end(case_sensitive_, false, false);

    set_margin_top(5);

    find_entry_.signal_changed().connect(sigc::mem_fun(this, &Search::on_entry_changed));
    find_entry_.signal_activate().connect(sigc::mem_fun(this, &Search::on_entry_activated));
    find_entry_.signal_key_press_event().connect(sigc::mem_fun(this, &Search::on_entry_key_press));

    find_next_button_.signal_clicked().connect(sigc::mem_fun(this, &Search::on_find_next_clicked));

    close_button_.signal_clicked().connect([&]() {
        signal_close_requested_();
    });

    find_entry_.add_events(Gdk::KEY_PRESS_MASK);
    case_sensitive_.signal_toggled().connect(sigc::mem_fun(this, &Search::on_case_sensitive_changed));

    auto context = Gtk::StyleContext::create();
    auto entry_path = find_entry_.get_path();
    context->set_path(entry_path);
    context->add_class("entry");

    default_entry_colour_ = context->get_color(Gtk::STATE_FLAG_FOCUSED);
}

void Search::on_entry_changed() {
    last_selected_match_ = -1;

    auto text = unicode(find_entry_.get_text().c_str());

    if(text.empty()) {
        find_entry_.override_color(default_entry_colour_, Gtk::STATE_FLAG_FOCUSED);
        toggle_replace(false);
        return;
    }

    std::vector<Gtk::TextBuffer::iterator> iters;
    int highlighted = highlight_all(text, iters);
    if(!highlighted) {
        find_entry_.override_color(Gdk::RGBA("#FF0000"), Gtk::STATE_FLAG_FOCUSED);
    } else {
        frame_->view().scroll_to(iters.at(0));
        find_entry_.override_color(default_entry_colour_, Gtk::STATE_FLAG_FOCUSED);
    }

    toggle_replace(highlighted > 0);
}

void Search::on_entry_activated() {
    this->find_next_button_.clicked();
}

bool Search::on_entry_key_press(GdkEventKey* event) {
    if(event->type == GDK_KEY_PRESS) {
        switch(event->keyval) {
            case GDK_KEY_Escape: {
                signal_close_requested_();
                return false;
            }
            break;
            default:
                break;
        }
    }
    return false;
}

void Search::locate_matches(const unicode& string) {
    matches_.clear();
    last_selected_match_ = -1;

    auto buf = this->buffer()->_gtk_buffer();
    auto start = buf->begin();
    Gtk::TextIter end;

    bool case_sensitive = case_sensitive_.get_active();

    while(start.forward_search(
              string.encode(), (case_sensitive) ? Gtk::TextSearchFlags(0) : Gtk::TEXT_SEARCH_CASE_INSENSITIVE,
              start, end)) {

        matches_.push_back(std::make_pair(start, end));
        start = buf->get_iter_at_offset(end.get_offset());
    }

}

int32_t Search::find_next_match(const Gtk::TextIter& start) {
    if(matches_.empty()) {
        return -1;
    }

    int i = 0;
    for(auto match: matches_) {
        if(match.first.get_offset() > start.get_offset()) {
            return i;
        }
        ++i;
    }

    //If we got here then there were no matches after the cursor, so just return the first one out
    //of all of them
    return 0;
}

void Search::on_replace_all_clicked() {
    auto text = find_entry_.get_text();
    auto replacement_text = replace_entry_.get_text();
    if(text == replacement_text) {
        //no-op
        return;
    }

    on_replace_clicked();

    while(!matches_.empty()) {
        on_replace_clicked();
    }
}

void Search::on_replace_clicked() {
    auto text = find_entry_.get_text();
    auto replacement_text = replace_entry_.get_text();
    if(text == replacement_text) {
        //no-op
        return;
    }

    std::pair<Gtk::TextIter, Gtk::TextIter> to_replace;
    if(last_selected_match_ > -1 && !matches_.empty()) {
        //If we had a selected match already, then replace that before moving on
        to_replace = matches_[last_selected_match_];


        auto buf = buffer()->_gtk_buffer();

        //Store the offset to the start of the selection set to be replaced
        auto offset = to_replace.first.get_offset();

        //Erase the selection
        buf->erase(to_replace.first, to_replace.second);

        //Build a new iterator from the stored offset, and insert the text there
        auto new_start = buf->get_iter_at_offset(offset);
        buf->insert(new_start, replacement_text);
    }

    //Find the next match
    on_find_next_clicked();
}

void Search::on_find_next_clicked() {
    if(!buffer()) {
        return;
    }

    auto buf = buffer()->_gtk_buffer();

    auto start = buf->get_iter_at_mark(buf->get_insert());

    auto text = unicode(find_entry_.get_text().c_str());

    locate_matches(text);

    auto next = find_next_match(start);
    last_selected_match_ = next;

    if(next > -1) {
        auto match = matches_[next];
        buf->select_range(match.first, match.second);
        frame_->view().scroll_to(match.first);
    }
}

int Search::highlight_all(const unicode& string) {
    std::vector<Gtk::TextBuffer::iterator> trash;
    return highlight_all(string, trash);
}

int Search::highlight_all(const unicode& string, std::vector<Gtk::TextBuffer::iterator>& start_iters) {
    auto buf = buffer()->_gtk_buffer();
    auto start = buf->begin();
    auto end_of_file = buf->end();
    auto end = start;

    bool case_sensitive = case_sensitive_.get_active();

    //Remove any existing highlights
    buf->remove_tag_by_name(SEARCH_HIGHLIGHT_TAG, start, end_of_file);

    if(string.empty()) {
        return 0;
    }

    int highlighted = 0;
    while(start.forward_search(
              string.encode(), (case_sensitive) ? Gtk::TextSearchFlags(0) : Gtk::TEXT_SEARCH_CASE_INSENSITIVE,
              start, end)) {
        start_iters.push_back(start);
        buf->apply_tag_by_name(SEARCH_HIGHLIGHT_TAG, start, end);
        start = buf->get_iter_at_offset(end.get_offset());
        ++highlighted;
    }
    return highlighted;
}

void Search::clear_highlight() {
    if(buffer()) {
        auto buf = buffer()->_gtk_buffer();
        buf->remove_tag_by_name(SEARCH_HIGHLIGHT_TAG, buf->begin(), buf->end());
    }
}

}
