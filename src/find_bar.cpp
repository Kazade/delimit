#include <kazbase/logging.h>
#include <cassert>
#include "find_bar.h"
#include "window.h"

namespace delimit {

const Glib::ustring SEARCH_HIGHLIGHT_TAG = "search_highlight";

FindBar::FindBar(Window& parent, Glib::RefPtr<Gtk::Builder>& builder):
    window_(parent) {

    build_widgets(builder);
    _connect_signals();

    find_bar_->show();
    find_bar_->set_reveal_child(false);
}

void FindBar::_connect_signals() {
    window_.signal_document_switched().connect(sigc::mem_fun(this, &FindBar::on_document_switched));
    close_button_->signal_clicked().connect(sigc::mem_fun(this, &FindBar::hide));
    replace_button_->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_replace_clicked));
    replace_all_button_->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_replace_all_clicked));
}

void FindBar::show() {
    auto buffer = window_.current_buffer();

    if(!buffer) {
        return;
    }

    find_bar_->set_reveal_child(true);
    find_entry_->grab_focus();

    toggle_replace(false);

    //If there is some text selected in the buffer, use that as the default text
    Gtk::TextIter start, end;

    if(buffer->buffer()->get_selection_bounds(start, end)) {
        find_entry_->set_text(buffer->buffer()->get_slice(start, end));
        find_entry_->select_region(0, -1);
    }
}

void FindBar::on_document_switched(DocumentView& buffer)  {
    //Create the buffer tag if necessary
    if(!buffer.buffer()->get_tag_table()->lookup(SEARCH_HIGHLIGHT_TAG)) {
        L_DEBUG("Creating search highlight tag");
        auto tag = buffer.buffer()->create_tag (SEARCH_HIGHLIGHT_TAG);
        tag->property_background() = "#1EBCFF";
    }
}

void FindBar::build_widgets(Glib::RefPtr<Gtk::Builder>& builder) {
    builder->get_widget("find_bar", find_bar_);
    builder->get_widget("find_entry", find_entry_);
    builder->get_widget("replace_entry", replace_entry_);
    builder->get_widget("find_next_button", find_next_button_);
    builder->get_widget("replace_button", replace_button_);
    builder->get_widget("replace_all_button", replace_all_button_);
    builder->get_widget("find_close_button", close_button_);
    builder->get_widget("case_sensitive", case_sensitive_);

    find_entry_->signal_changed().connect(sigc::mem_fun(this, &FindBar::on_entry_changed));
    find_entry_->signal_activate().connect(sigc::mem_fun(this, &FindBar::on_entry_activated));
    find_entry_->signal_key_press_event().connect(sigc::mem_fun(this, &FindBar::on_entry_key_press));

    find_next_button_->signal_clicked().connect(sigc::mem_fun(this, &FindBar::on_find_next_clicked));

    close_button_->signal_clicked().connect([&]() {
        signal_close_requested_();
    });

    find_entry_->add_events(Gdk::KEY_PRESS_MASK);
    case_sensitive_->signal_state_changed().connect(sigc::mem_fun(this, &FindBar::on_case_sensitive_changed));

    auto context = Gtk::StyleContext::create();
    auto entry_path = find_entry_->get_path();
    context->set_path(entry_path);
    context->add_class("entry");

    default_entry_colour_ = context->get_color(Gtk::STATE_FLAG_FOCUSED);
}

void FindBar::on_entry_changed() {
    last_selected_match_ = -1;

    auto text = unicode(find_entry_->get_text().c_str());

    if(text.empty()) {
        find_entry_->override_color(default_entry_colour_, Gtk::STATE_FLAG_FOCUSED);
        toggle_replace(false);
        return;
    }

    std::vector<Gtk::TextBuffer::iterator> iters;
    int highlighted = highlight_all(text, iters);
    if(!highlighted) {
        find_entry_->override_color(Gdk::RGBA("#FF0000"), Gtk::STATE_FLAG_FOCUSED);
    } else {
        window_.current_buffer()->view().scroll_to(iters.at(0));
        find_entry_->override_color(default_entry_colour_, Gtk::STATE_FLAG_FOCUSED);
    }

    toggle_replace(highlighted > 0);
}

void FindBar::on_entry_activated() {
    this->find_next_button_->clicked();
}

bool FindBar::on_entry_key_press(GdkEventKey* event) {
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

void FindBar::locate_matches(const unicode& string) {
    matches_.clear();
    last_selected_match_ = -1;

    auto buf = window_.current_buffer()->buffer();
    auto start = buf->begin();
    Gtk::TextIter end;

    bool case_sensitive = case_sensitive_->get_active();

    while(start.forward_search(
              string.encode(), (case_sensitive) ? Gtk::TextSearchFlags(0) : Gtk::TEXT_SEARCH_CASE_INSENSITIVE,
              start, end)) {

        matches_.push_back(std::make_pair(start, end));
        start = buf->get_iter_at_offset(end.get_offset());
    }

}

int32_t FindBar::find_next_match(const Gtk::TextIter& start) {
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

void FindBar::replace_text(Gtk::TextIter& start, Gtk::TextIter& end, Glib::ustring& replacement) {
    auto buf = window_.current_buffer()->buffer();

    if(buf->get_slice(start, end) == replacement) {
        return;
    }

    //Store the offset to the start of the selection set to be replaced
    auto offset = start.get_offset();

    //Erase the selection
    buf->erase(start, end);

    //Build a new iterator from the stored offset, and insert the text there
    auto new_start = buf->get_iter_at_offset(offset);
    buf->insert(new_start, replacement);
}

void FindBar::on_replace_all_clicked() {
    auto text = find_entry_->get_text();
    auto replacement_text = replace_entry_->get_text();

    /*
     * This is either clever or stupid...
     *
     * Basically, we can't just call locate_matches
     * and then iterate them because replacing the first will invalidate
     * all the iterators. So we have to call locate_matches after each replacement
     * until all the matches are gone. But there is an edge case, if we have a case-insensitive
     * match, and we replace the same text but with different capitalization (e.g. replace
     * 'cheese' with 'CHEESE') then the number of matches never changes, even when the replacements
     * happen - so we'd hit an infinte loop. So what we do is we keep track of the number of matches
     * if the number of matches changes (goes down) then we don't increment i to read the next match
     * we just keep replacing matches_[0] till they are all gone. If the number of matches doesn't change
     * then we increment 'i' each time so we eventually replace all the matches even if they are the same
     * case! Phew!!
     */

    locate_matches(unicode(text.c_str()));
    uint32_t i = 0;
    uint32_t last_matches_size = matches_.size();
    while(!matches_.empty() && i < matches_.size()) {
        replace_text(matches_[i].first, matches_[i].second, replacement_text);
        locate_matches(unicode(text.c_str()));
        if(matches_.size() == last_matches_size) {
            ++i;
        }
    }
}

void FindBar::on_replace_clicked() {
    auto text = find_entry_->get_text();
    auto replacement_text = replace_entry_->get_text();

    if(text == replacement_text) {
        //no-op
        return;
    }

    std::pair<Gtk::TextIter, Gtk::TextIter> to_replace;
    if(last_selected_match_ > -1 && !matches_.empty()) {
        //If we had a selected match already, then replace that before moving on
        to_replace = matches_[last_selected_match_];

        replace_text(to_replace.first, to_replace.second, replacement_text);
    }

    //Find the next match
    on_find_next_clicked();
}

void FindBar::on_find_next_clicked() {
    if(!window_.current_buffer()) {
        return;
    }

    auto buf = window_.current_buffer()->buffer();

    auto start = buf->get_iter_at_mark(buf->get_insert());

    auto text = unicode(find_entry_->get_text().c_str());

    locate_matches(text);

    auto next = find_next_match(start);
    last_selected_match_ = next;

    if(next > -1) {
        auto match = matches_[next];
        buf->select_range(match.first, match.second);
        window_.current_buffer()->view().scroll_to(match.first);
    }
}

int FindBar::highlight_all(const unicode& string) {
    std::vector<Gtk::TextBuffer::iterator> trash;
    return highlight_all(string, trash);
}

int FindBar::highlight_all(const unicode& string, std::vector<Gtk::TextBuffer::iterator>& start_iters) {
    auto buf = window_.current_buffer()->buffer();
    auto start = buf->begin();
    auto end_of_file = buf->end();
    auto end = start;

    bool case_sensitive = case_sensitive_->get_active();

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

void FindBar::clear_highlight() {
    if(window_.current_buffer()) {
        auto buf = window_.current_buffer()->buffer();
        buf->remove_tag_by_name(SEARCH_HIGHLIGHT_TAG, buf->begin(), buf->end());
    }
}

}
