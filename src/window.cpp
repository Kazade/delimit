#include <gtkmm.h>
#include <glibmm/i18n.h>
#include <gdkmm.h>
#include <thread>

#include "utils/sigc_lambda.h"
#include "autocomplete/provider.h"
#include "window.h"
#include "application.h"
#include "search/search_thread.h"
#include "utils.h"
#include "project_info.h"

#include "utils/unicode.h"
#include "utils/base_directory.h"

namespace delimit {

const int FRAME_LIMIT = 2;
const unicode UI_FILE = "delimit/ui/delimit.glade";

Window::Window():
    gtk_window_(nullptr),
    gtk_container_(nullptr),
    file_tree_scrolled_window_(nullptr),
    window_file_tree_(nullptr),
    buffer_new_(nullptr),
    buffer_open_(nullptr),
    folder_open_(nullptr),
    buffer_save_(nullptr),
    buffer_undo_(nullptr),
    main_paned_(nullptr),
    type_(WINDOW_TYPE_FILE),
    info_(new ProjectInfo()){

    L_DEBUG("Creating window with empty buffer");
    load_settings();


    file_tree_store_ = Gtk::TreeStore::create(file_tree_columns_);

    build_widgets();
    //Don't show the folder tree on FILE windows
    file_tree_scrolled_window_->get_parent()->remove(*file_tree_scrolled_window_);
}

Window::Window(const std::vector<Glib::RefPtr<Gio::File>>& files):
    gtk_window_(nullptr),
    gtk_container_(nullptr),
    file_tree_scrolled_window_(nullptr),
    window_file_tree_(nullptr),
    buffer_new_(nullptr),
    buffer_open_(nullptr),
    folder_open_(nullptr),
    buffer_save_(nullptr),
    buffer_undo_(nullptr),
    main_paned_(nullptr),
    type_(WINDOW_TYPE_FILE),
    info_(new ProjectInfo()){

    load_settings();

    file_tree_store_ = Gtk::TreeStore::create(file_tree_columns_);

    build_widgets();

    if(files.size() == 1 && files[0]->query_file_type() == Gio::FILE_TYPE_DIRECTORY) {
        type_ = WINDOW_TYPE_FOLDER;

        auto recent_manager = Gtk::RecentManager::get_default();
        recent_manager->add_item(_u("file://{0}").format(files[0]->get_path()).encode());

        rebuild_file_tree(files[0]->get_path());
        path_ = files[0]->get_path();

        info_->recursive_populate(path_);
        //awesome_bar_->repopulate_files();

        //Look for a .gitignore file in the directory
        auto ignore_file = kfs::path::join(path_.encode(), ".gitignore");
        L_DEBUG(_F("Checking for .gitignore at {0}...").format(ignore_file));
        if(kfs::path::exists(ignore_file)) {
            auto lines = read_file_lines(ignore_file);
            for(auto line: lines) {
                if(!line.strip().empty()) {
                    L_DEBUG(_F("Adding ignore glob: '{0}', length: {1}").format(line.strip(), line.strip().length()));
                    ignored_globs_.insert(line.strip());
                }
            }
        } else {
            L_DEBUG("not found.");
        }

    } else {
        type_ = WINDOW_TYPE_FILE;

        for(auto file: files) {
            open_document(file->get_path());
        }

        //Don't show the folder tree on FILE windows
        file_tree_scrolled_window_->get_parent()->remove(*file_tree_scrolled_window_);
    }

    header_bar_->set_title(_u("Delimit{0}").format((project_path().empty() ? "" : _u(" - ") + this->project_path())).encode());
}

void Window::show_awesome_bar(bool value) {
    if(value) {
        awesome_bar_->show();
        awesome_bar_->show_all_children();
    } else {
        awesome_bar_->hide();
    }
}

void Window::init_actions() {
    accel_group_ = Gtk::AccelGroup::create();
    buffer_new_->add_accelerator("clicked", accel_group_, GDK_KEY_N, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    buffer_open_->add_accelerator("clicked", accel_group_, GDK_KEY_O, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    folder_open_->add_accelerator("clicked", accel_group_, GDK_KEY_O, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK, Gtk::ACCEL_VISIBLE);
    buffer_save_->add_accelerator("clicked", accel_group_, GDK_KEY_S, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    //buffer_close_->add_accelerator("clicked", accel_group_, GDK_KEY_W, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

    _gtk_window().add_accel_group(accel_group_);

    action_group_ = Gtk::ActionGroup::create("MainActionGroup");

    add_global_action("search",
        Gtk::AccelKey(GDK_KEY_F, Gdk::CONTROL_MASK | Gdk::SHIFT_MASK),
        std::bind(&Window::begin_search, this, "")
    );

    add_global_action("find", Gtk::AccelKey(GDK_KEY_F, Gdk::CONTROL_MASK), [&]() {
        this->find_bar_->show();
    });

    add_global_action("awesome", Gtk::AccelKey(GDK_KEY_K, Gdk::CONTROL_MASK), [&]() {
        show_awesome_bar();
    });

    add_global_action("back_up", Gtk::AccelKey(GDK_KEY_Escape, Gdk::ModifierType(0)), [&]() {
        if(this->current_buffer() && this->current_buffer()->completion_visible()) {
            this->current_buffer()->hide_completion();
        } else {
            if(is_awesome_bar_visible()) {
                show_awesome_bar(false);
            } else {
                this->find_bar_->hide();
            }
        }
    });
}

void Window::clear_search_results() {
    for(auto child: search_results_->get_children()) {
        search_results_->remove(*child);
    }
}

void Window::begin_search(const std::string& within_directory) {
    gtk_search_window_->set_transient_for(*gtk_window_);

    if(type() == WINDOW_TYPE_FILE) {
        // Hide the glob matching options - we'll just be searching the open files
        excluding_glob_->set_sensitive(false);
        matching_glob_->set_sensitive(false);
    } else {
        excluding_glob_->set_sensitive(true);
        matching_glob_->set_sensitive(true);
        search_within_->set_filename(
            (within_directory.empty()) ? project_path().encode() : within_directory
        );
    }

    int response = gtk_search_window_->run();

    if(response == Gtk::RESPONSE_OK) {
        if(search_thread_) {
            //Stop any running search thread
            search_thread_->stop();
            search_thread_.reset();
        }

        set_task_active(0);
        set_task_in_progress(0);
        set_task_tabs_visible();
        clear_search_results();

        std::vector<unicode> files_to_search = info()->file_paths();
        unicode search_text = search_text_entry_->get_text().c_str();

        //Start the search thread
        search_thread_ = std::make_shared<SearchThread>(files_to_search, search_text, false, search_within_->get_filename());

        Glib::signal_idle().connect([&]() -> bool {
            int i = 10;
            while(i-- && search_thread_) {
                auto match = search_thread_->pop_result();
                if(!match) {
                    break;
                }

                Gtk::Expander* expander = Gtk::manage(new Gtk::Expander(match->filename.encode()));
                Gtk::ListBox* list_box = Gtk::manage(new Gtk::ListBox());
                list_box->set_valign(Gtk::ALIGN_START);
                list_box->signal_row_activated().connect([this](Gtk::ListBoxRow* row) {
                    Gtk::HBox* row_box = dynamic_cast<Gtk::HBox*>(row->get_children()[0]);
                    Gtk::Label* number_label = dynamic_cast<Gtk::Label*>(row_box->get_children()[0]);
                    Gtk::Label* string_label = dynamic_cast<Gtk::Label*>(row_box->get_children()[1]);

                    if(number_label && string_label) {
                        int line = unicode(number_label->get_text().c_str()).to_int();
                        unicode file = string_label->get_text().c_str();

                        open_document(file);
                        current_buffer()->scroll_to_line(line);
                    }
                });

                /*
                list_box->signal_focus_out_event().connect([list_box](GdkEventFocus*) -> bool {
                    auto selected = list_box->get_selected_row();
                    if(selected) {
                        selected->set_state(Gtk::STATE_NORMAL);
                    }

                    return false;
                });*/

                expander->set_margin_bottom(5);
                expander->add(*list_box);
                for(auto m: match->matches) {
                    Gtk::ListBoxRow* row = Gtk::manage(new Gtk::ListBoxRow());
                    Gtk::HBox* row_box = Gtk::manage(new Gtk::HBox());
                    Gtk::Label* number_label = Gtk::manage(new Gtk::Label(std::to_string(m.line), 0, 0.5));

                    row_box->set_margin_top(5);
                    row_box->set_margin_bottom(5);

                    /* This is so hacky! Here I'm adding a label, which contains the file path, then hiding it
                     * so I can hackily read it in the lambda callback above. I can't figure out how to bind
                     * this data to the widget. A cleaner way would be to store a map of ListBoxRow -> data
                     * but that's just as error prone :/
                     */
                    Gtk::Label* file_label = Gtk::manage(new Gtk::Label(match->filename.encode()));
                    file_label->set_no_show_all();
                    file_label->hide();
                    Gtk::Label* string_label = Gtk::manage(new Gtk::Label(m.text.encode(), 0, 0.5));

                    number_label->set_size_request(50, -1);
                    number_label->set_margin_left(15);
                    number_label->set_margin_right(10);
                    string_label->set_margin_right(5);
                    string_label->set_ellipsize(Pango::ELLIPSIZE_END);

                    row_box->set_homogeneous(false);
                    row_box->pack_start(*number_label, false, false);
                    row_box->pack_start(*file_label, false, false);
                    row_box->pack_start(*string_label, true, true);
                    row_box->set_vexpand();

                    row->add(*row_box);
                    list_box->add(*row);
                }
                search_results_->pack_start(*expander, true, true);
                search_results_->show_all();
            }

            if(search_thread_ && !search_thread_->running()) {
                search_thread_->join();
                search_thread_.reset();
                set_task_in_progress(0, false);
            }

            return search_thread_ && search_thread_->running();
        });
    }

    gtk_search_window_->hide();
}

void Window::cancel_search() {
    search_thread_->stop();
    search_thread_->join();
    search_thread_.reset();

    set_task_in_progress(0, false);
}

void Window::add_global_action(const unicode& name, const Gtk::AccelKey& key, std::function<void ()> func) {
    auto new_action = Gtk::Action::create(name.encode());
    new_action->signal_activate().connect(func);
    action_group_->add(new_action, key);
    new_action->set_accel_group(accel_group_);
    new_action->connect_accelerator();
}

void Window::load_settings() {
    unicode master_settings_file = fdo::xdg::find_data_file("delimit/settings.json").first;
    std::vector<unicode> paths;

    auto ret = fdo::xdg::find_user_data_file("delimit/settings.json");
    if(ret.second) {
        paths.push_back(ret.first);
    }

    if(this->type() == WINDOW_TYPE_FOLDER) {
        paths.push_back(kfs::path::join(project_path().encode(), ".delimit"));
    }

    if(!kfs::path::exists(master_settings_file.encode())) {
        throw std::runtime_error(_F("Unable to locate master settings at {0}").format(master_settings_file));
    }

    jsonic::loads(read_file_contents(master_settings_file).encode(), settings_);

    /* FIXME: Implement jsonic::Node::update
    for(auto path: paths) {
        if(kfs::path::exists(path)) {
            jsonic::Node local;
            jsonic::loads(local, read_file_contents(path));
            settings_.update(local);
        }
    } */
}

void close_current_document(Window* window) {
    window->current_buffer()->close();
}

void Window::build_widgets() {
    std::string ui_file = fdo::xdg::find_data_file(UI_FILE).first.encode();
    auto builder = Gtk::Builder::create_from_file(ui_file);

    find_bar_ = std::make_shared<FindBar>(*this, builder);
    awesome_bar_ = std::make_shared<AwesomeBar>(*this);

    builder->get_widget("search_window", gtk_search_window_);
    builder->get_widget("search_text_entry", search_text_entry_);
    builder->get_widget("main_window", gtk_window_);
    builder->get_widget("excluding_glob", excluding_glob_);
    builder->get_widget("matching_glob", matching_glob_);
    builder->get_widget("within_directory", search_within_);

    builder->get_widget("window_container", gtk_container_);
    builder->get_widget_derived("window_file_tree", window_file_tree_);
    builder->get_widget("file_tree_scrolled_window", file_tree_scrolled_window_);

    builder->get_widget("tasks_book", tasks_book_);
    builder->get_widget("tasks_hide", tasks_hide_button_);
    builder->get_widget("tasks_stop", tasks_stop_button_);
    builder->get_widget("tasks_progress", tasks_progress_);
    builder->get_widget("tasks_header", tasks_header_);
    builder->get_widget("tasks_buttons", tasks_buttons_);
    builder->get_widget("search_results", search_results_);
    builder->get_widget("content_pane", content_pane_);
    builder->get_widget("tasks_box", tasks_box_);

    // Remove the tasks box from the content pane at first
    content_pane_->remove(*tasks_box_);

    task_states_ = {
        TaskState(
            _u("Search Results"),
            false,
            [=]() { this->cancel_search(); }
        )
    };

    window_file_tree_->set_model(file_tree_store_);
    //window_file_tree_->append_column("Icon", file_tree_columns_.image);
    window_file_tree_->append_column("Name", file_tree_columns_.name);
    window_file_tree_->signal_row_activated().connect(sigc::mem_fun(this, &Window::on_signal_row_activated));
    window_file_tree_->signal_test_expand_row().connect(sigc::mem_fun(this, &Window::on_tree_test_expand_row));
    window_file_tree_->signal_search_directory().connect(std::bind(&Window::begin_search, this, std::placeholders::_1));

    Gtk::ListBox* open_files_list;
    builder->get_widget("open_files_list", open_files_list);
    open_files_list_ = std::make_shared<_Gtk::OpenFilesList>(*this, open_files_list);
    open_files_list_->signal_row_activated().connect(sigc::mem_fun(this, &Window::on_list_signal_row_activated));
    open_files_list_->signal_row_close_clicked().connect([&](DocumentView::ptr buffer) {
        buffer->close();
    });

    builder->get_widget("buffer_new", buffer_new_);
    builder->get_widget("buffer_open", buffer_open_);
    builder->get_widget("folder_open", folder_open_);
    builder->get_widget("buffer_save", buffer_save_);
    builder->get_widget("buffer_undo", buffer_undo_);
    builder->get_widget("buffer_redo", buffer_redo_);
    builder->get_widget("error_counter", error_counter_);
    builder->get_widget("window_pane", main_paned_);
    builder->get_widget("header_bar", header_bar_);
    builder->get_widget("no_files_alignment", no_files_alignment_);

    //THIS SUCKS SO BAD, 3.12 doesn't have a binding for GtkOverlay
    overlay_ = GTK_OVERLAY(gtk_overlay_new());
    g_object_ref(gtk_container_->gobj()); //Make sure we don't destroy the container

    Gtk::Box* parent = dynamic_cast<Gtk::Box*>(gtk_container_->get_parent());
    parent->remove(*gtk_container_); //Remove the container from the heirarchy
    gtk_container_add(GTK_CONTAINER(overlay_), GTK_WIDGET(gtk_container_->gobj())); //Add the container to the overlay
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay_), GTK_WIDGET(awesome_bar_->gobj()));
    parent->pack_start(*Glib::wrap(GTK_WIDGET(overlay_)), true, true, 0);
    parent->reorder_child(*Glib::wrap(GTK_WIDGET(overlay_)), 0);
    g_object_unref(gtk_container_->gobj()); //Undo reffing
    gtk_widget_show(GTK_WIDGET(overlay_));


    buffer_new_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_new_clicked));
    buffer_open_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_open_clicked));
    buffer_save_->signal_clicked().connect(sigc::hide_return(sigc::mem_fun(this, &Window::toolbutton_save_clicked)));
    folder_open_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_open_folder_clicked));
    buffer_undo_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_undo_clicked));
    buffer_redo_->signal_clicked().connect(sigc::mem_fun(this, &Window::toolbutton_redo_clicked));
    buffer_new_->set_icon_name("document-new");
    buffer_save_->set_icon_name("document-save");
    buffer_open_->set_icon_name("document-open");
    folder_open_->set_icon_name("folder-open");
    buffer_undo_->set_icon_name("edit-undo");
    buffer_redo_->set_icon_name("edit-redo");

    header_bar_->set_show_close_button();

    assert(gtk_window_);

    init_actions();

    //Style the error counter
    Glib::ustring style = "#error_counter { color: white; background-color: #ff0000; border-radius: 3px; font-weight: bold; }";
    auto css = Gtk::CssProvider::create();
    if(css->load_from_data(style)) {
        auto screen = Gdk::Screen::get_default();
        auto ctx = error_counter_->get_style_context();
        ctx->add_provider_for_screen(screen, css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    } else {
        L_ERROR("Unable to set the correct style for the error counter");
    }

    error_counter_->set_no_show_all();
    error_counter_->signal_toggled().connect([&]() {
        Gtk::Popover* popover = Gtk::manage(new Gtk::Popover());
        popover->set_relative_to(*error_counter_);
        popover->set_position(Gtk::POS_BOTTOM);

        Gtk::ListBox* box = Gtk::manage(new Gtk::ListBox());
        for(auto& error: displayed_errors_) {
            box->add(*Gtk::manage(new Gtk::Label(error.second.encode())));
        }

        popover->add(*box);
        popover->show_all();
    });

    clear_error_panel();

    std::string icon_file = fdo::xdg::find_data_file("delimit/delimit.svg").first.encode();
    gtk_window_->set_icon_from_file(icon_file);

    gtk_window_->maximize();

    gtk_window_->signal_delete_event().connect([&](GdkEventAny*) -> bool {
        auto copy = documents_;
        for(auto document: copy) {
            document->close();
        }

        return false;
    });

    tasks_book_->set_no_show_all();
    tasks_header_->set_no_show_all();
    tasks_buttons_->set_no_show_all();

    set_task_tabs_visible(false); //Hide on new window
    set_tasks_visible(false); //Hide the tasks on launch

    // Initialize existing tab buttons
    int i = 0;
    for(auto child: tasks_buttons_->get_children()) {
        Gtk::ToggleButton* button = dynamic_cast<Gtk::ToggleButton*>(child);

        // If a button is toggled, and it's now active, then show the tasks
        // and activate that tab. Otherwise, hide the tasks
        button->signal_toggled().connect([=]() {
            if(button->get_active()) {
                set_tasks_visible();
                set_task_active(i);

                for(auto other_child: tasks_buttons_->get_children()) {
                    if(other_child == child) continue;

                    Gtk::ToggleButton* other_button = dynamic_cast<Gtk::ToggleButton*>(child);
                    other_button->set_active(false);
                }
            } else {
                set_tasks_visible(false);
            }
        });
        ++i;
    }

    /* When the hide button is clicked, find the active button and set it inactive */
    tasks_hide_button_->signal_clicked().connect([=]() {
        for(auto child: tasks_buttons_->get_children()) {
            Gtk::ToggleButton* button = dynamic_cast<Gtk::ToggleButton*>(child);
            if(button->get_active()) {
                button->set_active(false);
                break;
            }
        }
    });    
}

void Window::set_task_tabs_visible(bool value) {
    tasks_buttons_->set_visible(value);
}

void Window::set_tasks_visible(bool value) {
    if(value && !content_pane_->get_child2()) {
        content_pane_->pack2(*tasks_box_);
    } else if(!value && content_pane_->get_child2()) {
        content_pane_->remove(*tasks_box_);
    }

    bool headers_were_visible = tasks_buttons_->get_visible();
    tasks_header_->set_visible(value);        
    tasks_progress_->set_visible(task_states_[active_task_].in_progress);

    tasks_book_->set_show_tabs(false);
    tasks_book_->set_current_page(active_task_);

    if(!headers_were_visible) {
        content_pane_->set_position(main_paned_->get_allocated_height() - 300);
    }
}

void Window::set_task_active(uint32_t index) {
    assert(index < task_states_.size());
    set_tasks_visible(true);

    Gtk::Widget* widget = tasks_buttons_->get_children().at(index);
    Gtk::ToggleButton* button = dynamic_cast<Gtk::ToggleButton*>(widget);
    if(!button->get_active()) {
        button->set_active(true);
    }

    active_task_ = index;
}

void Window::set_task_in_progress(uint32_t index, bool value) {
    task_states_[index].in_progress = value;

    if(index == active_task_) {
        if(task_states_[index].func && task_states_[index].in_progress) {
            tasks_stop_button_->show();

            if(tasks_stop_connection_) {
                tasks_stop_connection_.disconnect();
            }

            tasks_stop_connection_ = tasks_stop_button_->signal_clicked().connect(task_states_[index].func);
        } else {
            tasks_stop_button_->hide();
        }

        tasks_progress_->set_visible(task_states_[active_task_].in_progress);
    }
}

void Window::on_signal_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
    auto row = *(file_tree_store_->get_iter(path));

    if(!row[file_tree_columns_.is_folder]) {
        Glib::ustring full_path = row[file_tree_columns_.full_path];

        if(Gio::File::create_for_path(full_path)->query_file_type() == Gio::FILE_TYPE_REGULAR) {
            open_document(kfs::path::real_path(full_path.c_str()));
        }
    }
}

void Window::on_list_signal_row_activated(DocumentView::ptr buffer) {
    if(buffer) {
        activate_document(buffer);
    }
}

void Window::toolbutton_new_clicked() {
    new_document();
}

void Window::toolbutton_undo_clicked() {
    if(current_document_) {
        current_document_->buffer()->undo();
    }        
}

void Window::toolbutton_redo_clicked() {    
    if(current_document_) {
        current_document_->buffer()->redo();
    }

}

void Window::toolbutton_open_clicked() {
    Gtk::FileChooserDialog dialog(_gtk_window(), _("Open a file..."));

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    Glib::RefPtr<Gtk::FileFilter> filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);

    int result = dialog.run();
    switch(result) {
        case Gtk::RESPONSE_OK: {
            std::string filename = dialog.get_filename();
            open_document(filename);
        } default:
            break;
    }
}

void Window::toolbutton_open_folder_clicked() {
    Gtk::FileChooserDialog dialog(_("Open a folder..."), Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);

    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::OPEN, Gtk::RESPONSE_OK);

    int result = dialog.run();
    switch(result) {
        case Gtk::RESPONSE_OK: {
            std::string filename = dialog.get_filename();

            Gio::Application::type_vec_files files;
            files.push_back(Gio::File::create_for_path(filename));
            auto window = std::make_shared<delimit::Window>(files);
            Glib::RefPtr<Gtk::Application> app = gtk_window_->get_application();
            Glib::RefPtr<Application>::cast_dynamic(app)->add_window(window);

            //If we have no open documents, close this window
            if(documents_.empty() && type_ == WINDOW_TYPE_FILE) {
                gtk_window_->close();
            }
        } default:
            break;
    }
}

bool Window::toolbutton_save_clicked() {
    bool was_saved = false;

    Gtk::FileChooserDialog dialog(_gtk_window(), _("Save a file"), Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_do_overwrite_confirmation(true);
    dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
    dialog.add_button(Gtk::Stock::SAVE, Gtk::RESPONSE_OK);

    auto document = current_document_;

    if(document) {
        unicode path = document->path();
        if(document->is_new_file()) {
            int result = dialog.run();
            switch(result) {
                case Gtk::RESPONSE_OK:
                    path = dialog.get_filename();
                    break;
                default:
                    return was_saved;
            }
        }
        document->save(path);
        was_saved = true;
    }

    return was_saved;
}

int Window::new_file_count() const {
    int i = 0;
    for(auto buffer: documents_) {
        if(buffer->is_new_file()) {
            ++i;
        }
    }
    return i;
}

bool Window::on_tree_test_expand_row(const Gtk::TreeModel::iterator& iter, const Gtk::TreeModel::Path& path) {
    Gtk::TreeRow row = (*iter);

    //Remove all dummy rows
    std::vector<Gtk::TreeRowReference> dummy_nodes;
    for(auto& child: row->children()) {
        assert(child);

        bool val = child->get_value(file_tree_columns_.is_dummy);

        if(val) {
            dummy_nodes.push_back(Gtk::TreeRowReference(file_tree_store_, file_tree_store_->get_path(child)));
        }
    }

    if(!dummy_nodes.empty()) {
        //Build the node
        auto path = Glib::ustring(row[file_tree_columns_.full_path]);
        auto unicode_path = unicode(path.c_str());
        dirwalk(unicode_path, &row);

        for(auto ref: dummy_nodes) {
            file_tree_store_->erase(file_tree_store_->get_iter(ref.get_path()));//Erase the dummy
        }
    }

    return false;
}

void Window::update_error_panel(const ErrorList &errors) {
    auto count = errors.size();
    if(count == 0) {
        error_counter_->set_label("0");
        error_counter_->hide();
    } else {
        error_counter_->set_label(_u("{0}").format(count).encode());
        error_counter_->show();
    }

    displayed_errors_ = errors;
}

void Window::on_folder_changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &other, Gio::FileMonitorEvent event_type) {
    L_DEBUG("Detected folder event: " + file->get_path());

    if(event_type == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT || event_type == Gio::FILE_MONITOR_EVENT_DELETED || event_type == Gio::FILE_MONITOR_EVENT_CREATED) {
        L_INFO("Detected folder change: " + file->get_path());

        unicode folder_path = kfs::path::dir_name(file->get_path());

        auto it = tree_row_lookup_.find(folder_path);
        if(it != tree_row_lookup_.end()) {
            Gtk::TreeRow node = *(file_tree_store_->get_iter(it->second.get_path()));
            dirwalk(folder_path, &node);
        }
    }

    update_vcs_branch_in_tree();
    L_DEBUG("VCS branch and file tree updated");
}

void Window::update_vcs_branch_in_tree() {
    auto root = file_tree_store_->get_iter(Gtk::TreeStore::Path("0")); //Get the root node

    const Gtk::TreeRow& row = *root;

    if(!row.get_value(file_tree_columns_.is_folder)) {
        return;
    }

    unicode path = std::string(row.get_value(file_tree_columns_.full_path));
    if(kfs::path::exists(kfs::path::join(path.encode(), ".git"))) {
        unicode result = call_command(
            _u("git rev-parse --abbrev-ref HEAD"),
            path
        );

        if(!result.empty()) {
            unicode new_name = _u("{0} [{1}]").format(
                kfs::path::split(path.encode()).second,
                result.encode()
            );
            row.set_value(file_tree_columns_.name, Glib::ustring(new_name.encode()));
        }
    }
}

void Window::watch_directory(const unicode& path) {
    if(tree_monitors_.find(path) != tree_monitors_.end()) {
        return;
    }

    //Handle updates to the directory
    Glib::RefPtr<Gio::File> file = Gio::File::create_for_path(path.encode());
    auto monitor = file->monitor_directory();
    tree_monitors_[path] = monitor;
    monitor->signal_changed().connect(sigc::mem_fun(this, &Window::on_folder_changed));
}

void Window::unwatch_directory(const unicode& path) {
    if(tree_monitors_.find(path) == tree_monitors_.end()) {
        return;
    }

    Glib::RefPtr<Gio::FileMonitor> monitor = tree_monitors_.at(path);
    monitor->cancel();
    tree_monitors_.erase(path);
}

void Window::dirwalk(const unicode& path, const Gtk::TreeRow* node) {
    std::vector<std::string> files;

    bool path_is_dir = kfs::path::is_dir(kfs::path::real_path(path.encode()));
    if(!path_is_dir) {
        L_DEBUG("Not walking file: " + path.encode());
        return;
    }

    watch_directory(path);

    files = kfs::path::list_dir(kfs::path::real_path(path.encode()));
    std::sort(files.begin(), files.end());

    std::vector<std::string> directories;

    for(auto it = files.begin(); it != files.end();) {
        unicode f = (*it);
        auto real_path = kfs::path::real_path(kfs::path::join(path.encode(), f.encode()));

        if(real_path.empty()) {
            //Broken symlink or something
            it = files.erase(it);
            L_DEBUG(_F("Skipping broken file {0}").format(f));
            continue;
        }

        L_DEBUG(_F("Adding file: {0}").format(real_path));
        if(kfs::path::is_dir(real_path)) {
            L_DEBUG("...Is directory");
            directories.push_back((*it));
            it = files.erase(it);
        } else {
            ++it;
        }
    }

    directories.insert(directories.end(), files.begin(), files.end());
    files = directories;

    Gtk::TreeIter root_iter;

    if(!node) {
        L_DEBUG("Adding root node: " + path.encode());
        auto image = Gtk::IconTheme::get_default()->load_icon("folder", Gtk::ICON_SIZE_MENU);

        //If the node already exists for this path, don't add it again
        if(tree_row_lookup_.find(path) != tree_row_lookup_.end()) {
            root_iter = file_tree_store_->get_iter(tree_row_lookup_[path].get_path());
        } else {
            //Add a new node, store a reference against the path
            root_iter = file_tree_store_->append();
            tree_row_lookup_[path] = Gtk::TreeRowReference(file_tree_store_, file_tree_store_->get_path(root_iter));
        }

        assert(root_iter);

        node = &(*root_iter);
        node->set_value(file_tree_columns_.name, Glib::ustring(kfs::path::split(path.encode()).second));
        node->set_value(file_tree_columns_.full_path, Glib::ustring(path.encode()));
        node->set_value(file_tree_columns_.image, image);
        node->set_value(file_tree_columns_.is_folder, true);
        node->set_value(file_tree_columns_.is_dummy, false);
    }


    std::vector<unicode> existing_children;

    auto it = tree_row_lookup_.find(path);
    if(it != tree_row_lookup_.end()) {
        //This was an existing folder in the tree, so it might have children

        auto ref = (*it).second;
        for(auto child: (*file_tree_store_->get_iter(ref.get_path())).children()) {
            //Go through each child node, store any existing children
            if(!child->get_value(file_tree_columns_.is_dummy)) {
                //this isn't a dummy, so add the path to the existing_children
                existing_children.push_back(unicode(child->get_value(file_tree_columns_.full_path)));
            }
        }
    }

    L_DEBUG("Processing files");
    for(unicode f: files) {
        if(f == "." || f == "..") continue;

        if(f.starts_with(".") || f.ends_with(".pyc")) {
            //Ignore hidden files and folders
            continue;
        }

        unicode full_name = kfs::path::join(path.encode(), f.encode());

        //We've found one of the existing children, so remove it from the list
        existing_children.erase(std::remove(existing_children.begin(), existing_children.end(), full_name), existing_children.end());
/*
        bool ignore = false;
        for(auto gl: ignored_globs_) {
            if(glob::match(full_name, gl)) {
                //Ignore this file/folder if it matches the ignored globs
                ///// DISABLED UNTIL I FIGURE OUT WHAT BROKE ignore = true;
                break;
            }
        }
*/
        bool ignore = false;
        if(ignore) {
            L_DEBUG("Ignoring file as it's in .gitignore or similar: " + full_name.encode());
            continue;
        }

        bool is_folder = kfs::path::is_dir(kfs::path::real_path(full_name.encode()));

        auto image = Gtk::IconTheme::get_default()->load_icon(
            (is_folder) ? "folder" : "document-new",
            Gtk::ICON_SIZE_MENU
        );

        Gtk::TreeIter iter;

        //If the node already exists for this path, don't add it again
        if(tree_row_lookup_.find(full_name) != tree_row_lookup_.end()) {
            iter = file_tree_store_->get_iter(tree_row_lookup_[full_name].get_path());
        }

        if(!iter) {
            if(node) {
                L_DEBUG("Adding child node: " + full_name.encode());
                iter = file_tree_store_->append(node->children());
            } else {
                L_DEBUG("Adding root node: " + full_name.encode());
                iter = file_tree_store_->append();
            }
            tree_row_lookup_[full_name] = Gtk::TreeRowReference(file_tree_store_, file_tree_store_->get_path(iter));
        }

        assert(iter);

        const Gtk::TreeRow& row = (*iter);
        row.set_value(file_tree_columns_.name, Glib::ustring(f.encode()));
        row.set_value(file_tree_columns_.full_path, Glib::ustring(full_name.encode()));
        row.set_value(file_tree_columns_.image, image);
        row.set_value(file_tree_columns_.is_folder, is_folder);
        row.set_value(file_tree_columns_.is_dummy, false);

        if(is_folder && row->children().empty()) {
            //Add a dummy node under the folder
            const Gtk::TreeIter dummy_iter = file_tree_store_->append(row->children());
            assert(dummy_iter);

            const Gtk::TreeRow& dummy = (*dummy_iter);
            dummy.set_value(file_tree_columns_.is_dummy, true);
        }
    }

    L_DEBUG("Processing done");
    for(unicode file: existing_children) {
        L_DEBUG("Found file in need of removal: " + file.encode());
        file_tree_store_->erase(file_tree_store_->get_iter(tree_row_lookup_.at(file).get_path()));
        tree_row_lookup_.erase(file);
        unwatch_directory(file);
    }
}

void Window::rebuild_file_tree(const unicode& path) {
    file_tree_store_->clear();

    dirwalk(path, nullptr);

    window_file_tree_->expand_row(Gtk::TreeModel::Path("0"), false);

    update_vcs_branch_in_tree();
}

void Window::rebuild_open_list() {
    open_files_list_->clear();

    auto tmp = documents_;

    std::sort(tmp.begin(), tmp.end(), [](DocumentView::ptr lhs, DocumentView::ptr rhs) {
        return lhs->name().lower() < rhs->name().lower();
    });

    std::map<unicode, int> occurrance_count;

    for(auto buffer: tmp) {
        unicode name = buffer->name();
        if(occurrance_count.find(name) == occurrance_count.end()) {
            occurrance_count.insert(std::make_pair(name, 0));
        }

        occurrance_count[name] += 1;
    }

    for(auto buffer: tmp) {
        unicode name = buffer->name();

        if(occurrance_count[name] > 1) {
            name = _F("{0} ({1})").format(name, kfs::path::dir_name(buffer->path().replace(path_, "").lstrip("/").encode()));
        }

        _Gtk::OpenFilesEntry entry;

        entry.name = Glib::ustring(name.encode());
        entry.buffer = buffer;

        open_files_list_->add_entry(entry);
    }
}


void Window::on_document_modified(DocumentView& document) {
    buffer_save_->set_sensitive(document.buffer()->get_modified());

    unicode to_display = (document.is_new_file()) ? document.name() : document.path();

    if(type_ == WINDOW_TYPE_FOLDER && !path_.empty() && !document.path().empty()) {
        to_display = kfs::path::rel_path(to_display.encode(), path_.encode());
    }

    header_bar_->set_subtitle(
        _F("{0}{1}").format(
            to_display, document.buffer()->get_modified() ? "*": ""
        ).c_str()
    );
}

void Window::activate_document(DocumentView::ptr document) {
    if(current_document_ == document) {
        return;
    }

    gtk_container_->remove();
    gtk_container_->add(document->container());
    current_document_ = document;

    signal_document_switched_(*current_document_);
    on_document_modified(*document);

    //When we activate a document, make sure we update the linting
    Glib::signal_idle().connect_once([document]() {
        document->run_linters_and_stuff(true);
    });

    set_task_tabs_visible(true);
}

void Window::append_document(DocumentView::ptr new_document) {
    //Watch for changes to the buffer
    new_document->signal_modified_changed().connect(
        sigc::mem_fun(this, &Window::on_document_modified)
    );

    new_document->signal_closed().connect(
        sigc::mem_fun(this, &Window::close_document)
    );

    documents_.push_back(new_document);

}

void Window::new_document() {
    DocumentView::ptr new_document = std::make_shared<DocumentView>(*this);

    append_document(new_document);
    activate_document(new_document);

    rebuild_open_list();
}

void Window::close_document(DocumentView& document) {
    DocumentView::ptr prev, this_buf;
    for(uint32_t i = 0; i < documents_.size(); ++i) {
        auto buf = documents_[i];

        if(buf.get() == &document) {
            this_buf = buf;
            if(!prev && i + 1 < documents_.size()) {
                //We closed the first file in the list, so point to the following one
                //if we can
                prev = documents_[i + 1];
            }
            break;
        }

        prev = buf;
    }

    if(prev) {
        activate_document(prev);
    }

    //erase the buffer from the open buffers
    documents_.erase(std::remove(documents_.begin(), documents_.end(), this_buf), documents_.end());

    //Redisplay the "no files" message when we close the last open file
    if(documents_.empty()) {
        gtk_container_->remove();
        gtk_container_->add(*no_files_alignment_);
        header_bar_->set_subtitle("");

        set_task_tabs_visible(false);
        set_tasks_visible(false);
        find_bar_->hide();
    }

    rebuild_open_list();
}

void Window::open_document(const unicode& file_path) {
    for(auto buffer: documents_) {
        if(buffer->path() == file_path) {
            //Just activate, don't open!
            activate_document(buffer);
            return;
        }
    }

    unicode name = kfs::path::split(file_path.encode()).second;

    DocumentView::ptr buffer = std::make_shared<DocumentView>(*this, file_path);

    append_document(buffer);
    activate_document(buffer);
    rebuild_open_list();
}


void Window::set_undo_enabled(bool value) {
    buffer_undo_->set_sensitive(value);
}

void Window::set_redo_enabled(bool value) {
    buffer_redo_->set_sensitive(value);
}


}
