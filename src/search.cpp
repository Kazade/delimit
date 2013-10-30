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
    find_entry_.signal_key_press_event().connect(sigc::mem_fun(this, &Search::on_entry_key_press));

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
    auto text = unicode(find_entry_.get_text().c_str());

    if(text.empty()) {
        find_entry_.override_color(default_entry_colour_, Gtk::STATE_FLAG_FOCUSED);
        toggle_replace(false);
        return;
    }

    int highlighted = highlight_all(text);
    if(!highlighted) {
        find_entry_.override_color(Gdk::RGBA("#FF0000"), Gtk::STATE_FLAG_FOCUSED);
    } else {
        find_entry_.override_color(default_entry_colour_, Gtk::STATE_FLAG_FOCUSED);
    }

    toggle_replace(highlighted > 0);
}

bool Search::on_entry_key_press(GdkEventKey* event) {
    if(event->type == GDK_KEY_PRESS) {
        switch(event->keyval) {
            case GDK_KEY_Escape: {
                signal_close_requested_();
                return false;
            }
            break;
            case GDK_KEY_Return: {
                find_next_button_.clicked();
                return false;
            }
            break;
            default:
                break;
        }
    }
    return false;
}

int Search::highlight_all(const unicode& string) {
    auto buffer = buffer_->_gtk_buffer();
    auto start = buffer->begin();
    auto end_of_file = buffer->end();
    auto end = start;

    bool case_sensitive = case_sensitive_.get_active();

    //Remove any existing highlights
    buffer->remove_tag_by_name(SEARCH_HIGHLIGHT_TAG, start, end_of_file);

    if(string.empty()) {
        return 0;
    }

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

void Search::clear_highlight() {
    if(buffer_) {
        auto buffer = buffer_->_gtk_buffer();
        buffer->remove_tag_by_name(SEARCH_HIGHLIGHT_TAG, buffer->begin(), buffer->end());
    }
}

}
