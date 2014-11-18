#include <kazbase/logging.h>
#include "document_view.h"
#include "window.h"
#include "application.h"
#include "utils/indentation.h"

namespace delimit {

DocumentView::DocumentView(Window& window):
    window_(window) {

    create_buffer();

    build_widgets();
    connect_signals();

    create_new_file();
}

DocumentView::DocumentView(Window& window, const unicode& filename):
    window_(window) {
    create_buffer();

    build_widgets();
    connect_signals();

    open_file(filename);
}

unicode DocumentView::name() const {
    return os::path::split(path()).second;
}

unicode DocumentView::path() const {
    return file_->get_path();
}

void DocumentView::create_buffer() {
    if(!buffer_) {
        buffer_ = Gsv::Buffer::create();
        buffer_->signal_modified_changed().connect([&]() {
            signal_modified_changed_(*this);
        });
    }
}

gchar* get_tooltip(GtkSourceMarkAttributes *attrs,
                   GtkSourceMark *mark,
                   GtkSourceView *view) {
    void* user_data = g_object_get_data(G_OBJECT(mark), "linter_message");
    if(user_data) {
        unicode* uniptr = (unicode*) user_data;
        std::string ret = uniptr->encode();
        Glib::ustring final = ret;
        return g_strdup_printf("%s", final.c_str());
    }

    return g_strdup_printf("No message");
}

bool DocumentView::completion_visible() const {
    return completion_visible_;
}

void DocumentView::hide_completion() {
    auto comp = view_.get_completion();
    if(comp) {
        comp->hide();
    }
}

void DocumentView::build_widgets() {
    scrolled_window_.add(view_);
    view_.set_buffer(buffer_);

    auto manager = Gsv::StyleSchemeManager::get_default();
    view_.get_source_buffer()->set_style_scheme(manager->get_scheme("delimit"));

    auto provider = Gsv::CompletionWords::create("", Glib::RefPtr<Gdk::Pixbuf>());
    provider->register_provider(buffer_);
    provider->property_priority().set_value(1);
    view_.get_completion()->add_provider(provider);
    view_.get_completion()->signal_show().connect([&]() { completion_visible_ = true; });
    view_.get_completion()->signal_hide().connect([&]() { completion_visible_ = false; });

    auto coverage_attrs = Gsv::MarkAttributes::create();
    Gdk::RGBA coverage_colour;
    coverage_colour.set_rgba(1.0, 0.5, 0, 0.12);
    coverage_attrs->set_background(coverage_colour);
    view_.set_mark_attributes("coverage", coverage_attrs, 0);

    auto linter_attrs = Gsv::MarkAttributes::create();
    linter_attrs->set_icon_name("dialog-error");
    g_signal_connect(linter_attrs->gobj(), "query-tooltip-markup", G_CALLBACK(get_tooltip), view_.gobj());
    view_.set_mark_attributes("linter", linter_attrs, 10);

    auto breakpoint = Gsv::MarkAttributes::create();
    breakpoint->set_icon_name("stop");
    view_.set_mark_attributes("breakpoint", breakpoint, 11);
    view_.add_events(Gdk::BUTTON_PRESS_MASK);
    view_.signal_button_press_event().connect([&](GdkEventButton* evt) -> bool {

        if(buffer_->get_language()->get_name() != "Python") {
            return false;
        }

        if(evt->type == GDK_2BUTTON_PRESS && evt->window == view_.get_window(Gtk::TEXT_WINDOW_LEFT)->gobj()) {
            //The user clicked within the left gutter
            if(evt->button == 1) {
                //Left click
                int x_buf, y_buf;

                view_.window_to_buffer_coords(
                    Gtk::TEXT_WINDOW_LEFT,
                    int(evt->x), int(evt->y),
                    x_buf, y_buf
                );

                //Line bounds
                Gtk::TextBuffer::iterator iter;
                int line_top;
                view_.get_line_at_y(iter, y_buf, line_top);
                auto mark_list = buffer_->get_source_marks_at_iter(iter, "breakpoint");
                if(!mark_list.empty()) {
                    for(auto& mark: mark_list) {
                        buffer_->delete_mark(mark);
                    }

                    auto end_iter = iter;
                    end_iter.forward_line();

                    unicode line = buffer_->get_slice(iter, end_iter).c_str();
                    if(line.contains("import ipdb; ipdb.set_trace()")) {
                        buffer_->erase(iter, end_iter);
                    }
                } else {
                    buffer_->create_source_mark("breakpoint", iter);

                    auto end_iter = iter;
                    end_iter.forward_line();

                    unicode line_text = buffer_->get_slice(iter, end_iter).c_str();

                    uint32_t i = 0;
                    for(; i < line_text.length(); ++i) {
                        if(line_text[i] != ' ' && line_text[i] != '\t') {
                            break;
                        }
                    }

                    auto indentation = line_text.slice(nullptr, i);
                    auto replacement_text = _u("{0}import ipdb; ipdb.set_trace();\n").format(indentation);
                    buffer_->insert(iter, replacement_text.encode());
                }
            }
        }
        return false;
    });

    apply_settings("text/plain");

    scrolled_window_.show_all();
}

void DocumentView::handle_file_deleted() {
    L_DEBUG("Processing deleted file");

    if(!os::path::exists(path())) {
        Gtk::MessageDialog dialog(window_._gtk_window(), "File Deleted", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        dialog.set_message(_u("The file <i>{0}</i> has been deleted").format(name()).encode(), true);
        dialog.set_secondary_text("Do you want to close this file?");
        int response = dialog.run();
        switch(response) {
            case Gtk::RESPONSE_YES:
                close();
            break;
            case Gtk::RESPONSE_NO:
                //If they didn't want to close the file, mark this file as modified from on disk
                buffer()->set_modified(true);
            break;
            default:
                return;
        }
    }
}

void DocumentView::handle_file_change_signal(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event) {
    unicode event_type;
    switch((int)event) {
        case 0: event_type = "CHANGED"; break;
        case 1: event_type = "CHANGES_DONE_HINT"; break;
        case 2: event_type = "DELETED"; break;
        case 3: event_type = "CREATED"; break;
        case 4: event_type = "ATTRIBUTE_CHANGED"; break;
        case 5: event_type = "PRE_UNMOUNT"; break;
        case 6: event_type = "UNMOUNTED"; break;
        case 7: event_type = "MOVED"; break;
        default: event_type = "UNKNOWN";
    }

    L_DEBUG(_u("Open file change event: {0}").format(event_type).encode());

    if(event == Gio::FILE_MONITOR_EVENT_DELETED) {
        Glib::signal_idle().connect_once(sigc::mem_fun(this, &DocumentView::handle_file_deleted));
    }  else if(event == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
        std::string etag = file->query_info(G_FILE_ATTRIBUTE_ETAG_VALUE)->get_etag();

        //Ignore the event if the etag matches one that we've saved
        if(etag != file_etag_) {
            L_DEBUG("etag doesn't match, so prompting:" + etag + " <> " + file_etag_);

            Gtk::MessageDialog dialog(window_._gtk_window(), "File Changed", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);

            dialog.set_message(_u("The file <i>{0}</i> has changed outside Delimit").format(os::path::split(file->get_path()).second).encode(), true);
            dialog.set_secondary_text("Do you want to reload?");
            dialog.add_button("Close", Gtk::RESPONSE_CLOSE);

            //Only display the dialog if we were modified otherwise default to
            //reloading automatically
            int response = Gtk::RESPONSE_YES;
            if(buffer()->get_modified()) {
                response = dialog.run();
            }

            switch(response) {
                case Gtk::RESPONSE_YES:
                    reload();
                break;
                case Gtk::RESPONSE_CLOSE:
                    //Close
                    close();
                break;
                case Gtk::RESPONSE_NO:
                    //The user didn't want to reload, so mark the file as modified
                    //so that they can see it doesn't match what's on disk
                    buffer()->set_modified(true);
                break;
                default:
                    return;
            }
        }
    }
}

void DocumentView::file_changed(const GioFilePtr& file, const GioFilePtr& other_file, Gio::FileMonitorEvent event) {
    handle_file_change_signal(file, other_file, event);
}

void DocumentView::connect_signals() {

}

bool DocumentView::is_new_file() const {
    return !os::path::exists(file_->get_path());
}

void DocumentView::set_file(GioFilePtr file) {
    disconnect_file_monitor();

    if(!file) {
        create_new_file();
    }

    file_ = file;

    if(!is_new_file()) {
        connect_file_monitor();
        reload();
    } else {
        file_etag_ = ""; //Wipe out the etag if this is a new file
    }
}

void DocumentView::trim_trailing_newlines() {
    //FIXME: Don't strip whitespace if the language doesn't like it (e.g. 'diff')
    auto buffer_end = buffer()->end();
    if(buffer_end.starts_line()) {
        auto itr = buffer_end;
        while(itr.backward_line()) {
            if(!itr.ends_line()) {
                itr.forward_to_line_end();
                break;
            }
        }
        buffer()->erase(itr, buffer_end);
    }

    auto current = buffer()->get_insert()->get_iter().get_offset();
    buffer()->insert(buffer()->end(), "\n"); //Make sure we have a newline at the end
    buffer()->place_cursor(buffer()->get_iter_at_offset(current));
}

bool is_whitespace(Gtk::TextIter& iter) {
    for(auto ch: std::string("\t\v\f ")) {
        if(iter.get_char() == (gunichar) ch) {
            return true;
        }
    }

    return false;
}

void DocumentView::trim_trailing_whitespace() {
    //FIXME: Don't strip whitespace if the language doesn't like it (e.g. 'diff')
    auto current = buffer()->begin();

    while(current != buffer()->end()) {
        current.forward_to_line_end(); //Move to the newline character
        auto line_end = current; //Store the end of the line
        while(is_whitespace(--current)) {} //Move backward until we get to non-whitespace

        current = buffer()->erase(++current, line_end);

        current.forward_line(); //Move to the next line, or the end of the final line
    }
}

void DocumentView::save(const unicode& path) {
    L_DEBUG("Saving file: " + path.encode());
    trim_trailing_newlines();
    trim_trailing_whitespace();

    Glib::ustring text = buffer()->get_text();

    if(file_->get_path() == path.encode()) {
        //FIXME: Use entity tag arguments to make sure that the file
        //didn't change since the last time we saved
        file_->replace_contents(std::string(text.c_str()), "", file_etag_);
        connect_file_monitor();
    } else {
        auto file = Gio::File::create_for_path(path.encode());
        if(!os::path::exists(path)) {
            file->create_file();
        }
        file->replace_contents(text, "", file_etag_);
        connect_file_monitor();
    }

    buffer()->set_modified(false);

    apply_settings(guess_mimetype()); //Make sure we update the settings when we've saved the file

    window_.rebuild_open_list();
    run_linters_and_stuff();
}

void DocumentView::reload() {
    file_etag_ = file_->query_info(G_FILE_ATTRIBUTE_ETAG_VALUE)->get_etag();
    std::function<void (Glib::RefPtr<Gio::AsyncResult>)> func = std::bind(&DocumentView::finish_read, this, file_, std::placeholders::_1);
    file_->load_contents_async(func);
}

Glib::RefPtr<Gsv::Language> guess_language_from_file(const GioFilePtr& file) {
    Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
    Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

    if(lang) {
        L_DEBUG(_u("Detected {0}").format(lang->get_name()));
    } else {
        //Extra checks

        std::vector<char> buffer(1024);
        file->read()->read(&buffer[0], 1024);

        auto line = unicode(buffer.begin(), buffer.end()).split("\n")[0].lower();
        auto language_ids = lm->get_language_ids();

        if(line.starts_with("#!")) {
            if(line.contains("python")) {
                lang = lm->get_language("python");
            } else {
                L_INFO(_u("Unrecognized #! {}").format(line));
            }
        }
    }
    return lang;
}

void DocumentView::apply_language_to_buffer(const Glib::RefPtr<Gsv::Language>& language) {
    if(!language) {
        return;
    }

    buffer_->set_language(language);
}

void DocumentView::run_linters_and_stuff(bool force) {
    if(!force && window().current_buffer().get() != this) {
        //Only run linters on the active document
        return;
    }

    apply_language_to_buffer(guess_language_from_file(file_));
    apply_settings(guess_mimetype()); //Make sure we update the settings when we've reloaded the file

    //Check for coverage stats
    auto language = buffer()->get_language();
    unicode name = (language) ? unicode(language->get_name()) : "";
    if(name == "Python") {
        if(!coverage_) {
            std::cout << "COVERAGE: Creating coverage" << std::endl;
            coverage_ = std::make_shared<coverage::PythonCoverage>();
            //Connect to the coverage updated signal and re-run this function if that happens
            coverage_->signal_coverage_updated().connect(std::bind(&DocumentView::run_linters_and_stuff, this, false));
        }
        coverage_->apply_to_document(this);

        linter_ = std::make_shared<linter::PythonLinter>();
        linter_->apply_to_document(this);

    } else {
        if(coverage_) {
           coverage_->clear_document(this);
           coverage_.reset();
        }

        if(linter_) {
           linter_->clear_document(this);
           linter_.reset();
        }
    }
}

void DocumentView::detect_and_apply_indentation() {
    if(current_settings_.has_key("detect_indentation") && current_settings_["detect_indentation"].get_bool()) {
        auto indent = detect_indentation(std::string(buffer_->get_text()));

        if(indent.first == INDENT_TABS) {
            view_.set_insert_spaces_instead_of_tabs(false);
        } else {
            view_.set_indent_width(indent.second);
            view_.set_insert_spaces_instead_of_tabs(true);
        }
    }
}

void DocumentView::finish_read(Glib::RefPtr<Gio::File> file, Glib::RefPtr<Gio::AsyncResult> res) {
    char* output;
    gsize size;

    file->load_contents_finish(res, output, size);

    Glib::ustring result(output);

    buffer_->begin_not_undoable_action();
    buffer_->set_text(result);
    buffer_->set_modified(false);
    buffer_->end_not_undoable_action();

    signal_loaded_(*this);

    detect_and_apply_indentation();
    std::function<void ()> func = std::bind(&DocumentView::run_linters_and_stuff, this, false);
    Glib::signal_idle().connect_once(func);
}

void DocumentView::connect_file_monitor() {
    if(!file_monitor_) {
        L_DEBUG("Connecting file monitor");
        file_monitor_ = file_->monitor_file();
        file_monitor_->signal_changed().connect(sigc::mem_fun(this, &DocumentView::file_changed));
    }
}

void DocumentView::disconnect_file_monitor() {
    if(file_monitor_) {
        L_DEBUG("Disconnecting existing file monitor");
        file_monitor_->cancel();
        file_monitor_.reset();
    }
}

void DocumentView::create_new_file() {
    /*
     *  Creates a new file. We create a Gio::File for a path in the tempfiles directory
     *  but don't actually create a file there. That way the lack of existence of the path
     *  allows us to determine whether the file is new, or has been deleted on disk
     */

    auto temp_files = tempfiles_directory();
    int existing_count = window_.new_file_count();

    //FIXME: This will create multiple files with the same path if we have untitled
    //files in two windows. I'm not sure if this is a problem or not considering these
    //paths are just placeholders. In future it may be!
    auto untitled_filename = _u("Untitled {0}").format(existing_count + 1);
    auto full_path = os::path::join(temp_files, untitled_filename);
    auto file = Gio::File::create_for_path(full_path.encode());

    set_file(file);

    buffer_->set_modified(true);
}

void DocumentView::open_file(const unicode& filename) {
    auto file = Gio::File::create_for_path(filename.encode());
    set_file(file);
}

void DocumentView::close() {
    //Display a prompt encourage saving unless the file is new and empty
    if(buffer() && buffer()->get_modified() && !(is_new_file() && buffer_->get_text().size() == 0)) {
        Gtk::MessageDialog dialog(window_._gtk_window(), "Do you want to save your work?", true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_YES_NO, true);
        dialog.set_message(_u("The file <i>{0}</i> has been changed since it was last saved.").format(name()).encode(), true);
        dialog.set_secondary_text("Do you want to save the file?");
        int response = dialog.run();
        switch(response) {
            case Gtk::RESPONSE_YES: {
                bool was_saved = window_.toolbutton_save_clicked();
                if(!was_saved) {
                    L_DEBUG("User cancelled saving a file, so we won't close the buffer");
                    return;
                }
            }
            break;
            default:
                break;
        }
    }

    disconnect_file_monitor();
    signal_closed_(*this);
}

std::pair<unicode, int> DocumentView::get_font_name_and_size_from_dconf() {
    auto settings = Gio::Settings::create("org.gnome.desktop.interface");
    auto font_name = settings->get_string("monospace-font-name");

    auto parts = unicode(font_name).split(" ");
    int size = 10;
    try {
        //Assume that the last part is the font size, if it's not
        //then fall back to size 10 and don't alter the font name
        size = parts.back().to_int();
        parts.pop_back();

        //FIXME: This is a hack to remove the style from the name...
        if(parts.back() == "Medium") {
            parts.pop_back();
        }
        font_name = _u(" ").join(parts).encode();
    } catch(std::exception& e) {}

    return std::make_pair(unicode(std::string(font_name)), size * PANGO_SCALE);
}

unicode DocumentView::guess_mimetype() const {
    if(is_new_file()) {
        return "text/plain";
    }

    auto info = file_->query_info(G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE);
    return info->get_content_type();
}

void DocumentView::apply_settings(const unicode& mimetype) {
    L_INFO(_u("Applying settings for: ") + mimetype);

    view_.set_left_margin(4);
    view_.set_right_margin(4);
    view_.set_show_line_marks(true);
    view_.set_smart_home_end(Gsv::SMART_HOME_END_AFTER);

    //Load any settings from the settings file
    json::JSON default_settings = window_.settings()["default"];

    if(window_.settings().has_key(mimetype)) {
        default_settings.update(window_.settings()[mimetype]);
    }

    view_.set_show_line_numbers(default_settings["show_line_numbers"].get_bool());
    view_.set_auto_indent(default_settings["auto_indent"].get_bool());

    if(!default_settings["detect_indentation"].get_bool()) {
        view_.set_tab_width(default_settings["tab_width"].get_number());
        view_.set_insert_spaces_instead_of_tabs(default_settings["insert_spaces_instead_of_tabs"].get_bool());
    }

    Gsv::DrawSpacesFlags flags;
    if(default_settings["draw_whitespace_spaces"].get_bool()) {
        flags |= Gsv::DRAW_SPACES_SPACE;
    }
    if(default_settings["draw_whitespace_tabs"].get_bool()) {
        flags |= Gsv::DRAW_SPACES_TAB;
    }

    view_.set_draw_spaces(flags);
    view_.set_highlight_current_line(default_settings["highlight_current_line"].get_bool());

    //Load any font setting overrides
    Pango::FontDescription fdesc;

    auto font_details = get_font_name_and_size_from_dconf();
    fdesc.set_family(font_details.first.encode());
    fdesc.set_size(font_details.second);

    if(default_settings.has_key("override_font_family")) {
        fdesc.set_family(default_settings["override_font_family"].get().encode());
    }

    if(default_settings.has_key("override_font_size")) {
        fdesc.set_size(default_settings["override_font_size"].get_number() * PANGO_SCALE);
    }

    view_.override_font(fdesc);
    current_settings_ = default_settings;
}

}
