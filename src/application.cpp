#include <glibmm/i18n.h>

#include "utils/kfs.h"
#include "application.h"
#include "window.h"
#include "autocomplete/provider.h"
#include "utils/base_directory.h"
#include "utils/kazlog.h"

namespace delimit {

void clear_tempfiles_directory() {
    auto tempfiles_dir = tempfiles_directory();
    for(auto file: kfs::path::list_dir(tempfiles_dir.encode())) {
        auto path = kfs::path::join(tempfiles_dir.encode(), file);
        if(!kfs::path::is_dir(path)) {
            kfs::remove(path);
        }
    }
}

unicode tempfiles_directory() {
    auto data_home = fdo::xdg::get_data_home();
    auto temp_files = kfs::path::join(data_home.encode(), "delimit/tempfiles");

    kfs::make_dirs(temp_files);

    return temp_files;
}

Application::Application(int& argc, char**& argv, const Glib::ustring& application_id, Gio::ApplicationFlags flags):
    Gtk::Application(argc, argv, application_id, flags) {

    Glib::set_application_name("Delimit");

    g_object_set(gtk_settings_get_default(),
        "gtk-application-prefer-dark-theme", TRUE,
        NULL);

    kazlog::get_logger("/")->add_handler(kazlog::Handler::ptr(new kazlog::StdIOHandler));

    signal_startup().connect(sigc::mem_fun(this, &Application::on_signal_startup));
    signal_open().connect(sigc::mem_fun(this, &Application::on_signal_open));

    for(int i = 0; i < argc; ++i) {
        args_.push_back(argv[i]);
    }

    Gsv::init();
}

Glib::RefPtr<Application> Application::create(
        int& argc, char**& argv,
        const Glib::ustring& application_id,
        Gio::ApplicationFlags flags) {

    return Glib::RefPtr<delimit::Application>(new delimit::Application(argc, argv, application_id, flags));
}

void Application::add_window(Window::ptr window) {    
    windows_.push_back(window);    
    add_window(window->_gtk_window());
    window->_gtk_window().show_all();
}

void Application::on_signal_open(const Gio::Application::type_vec_files& files, const Glib::ustring& line) {
    L_INFO(_F("on_signal_open {0}").format(line));

    if(files.empty()) {
        return;
    }

    auto window = std::make_shared<delimit::Window>(files);
    add_window(window);
}

void Application::action_open_folder(const Glib::VariantBase&) {
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
            add_window(window);
        } default:
            break;
    }
}

void Application::action_open_file(const Glib::VariantBase&) {
    Gtk::FileChooserDialog dialog(_("Open a file..."));

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

            Gio::Application::type_vec_files files;
            files.push_back(Gio::File::create_for_path(filename));
            auto window = std::make_shared<delimit::Window>(files);
            add_window(window);
        } default:
            break;
    }
}

void Application::action_quit(const Glib::VariantBase&) {
    quit();
}

void Application::on_startup() {
    Gtk::Application::on_startup();

    clear_tempfiles_directory();

    //Create the application menu
    app_menu_ = Gio::Menu::create();
    app_menu_->append(_("_Open File..."), "app.open_file");
    app_menu_->append(_("Open _Folder..."), "app.open_folder");
    app_menu_->append(_("_Quit"), "app.quit");
    set_app_menu(app_menu_);

    auto open_file_action = Gio::SimpleAction::create("open_file");
    open_file_action->signal_activate().connect(sigc::mem_fun(this, &Application::action_open_file));

    auto open_folder_action = Gio::SimpleAction::create("open_folder");
    open_folder_action->signal_activate().connect(sigc::mem_fun(this, &Application::action_open_folder));

    auto quit_action = Gio::SimpleAction::create("quit");
    quit_action->signal_activate().connect(sigc::mem_fun(this, &Application::action_quit));

    add_action(open_file_action);
    add_action(open_folder_action);
    add_action(quit_action);
}

void Application::on_signal_startup() {
    L_INFO("on_signal_startup");

    //Add our schemes to the list of available ones
    auto manager = Gsv::StyleSchemeManager::get_default();
    std::string ui_file = fdo::xdg::find_data_file("delimit/schemes").first.encode();
    manager->prepend_search_path(ui_file);

    if(args_.size() == 1) {
        auto window = std::make_shared<delimit::Window>();
        add_window(window);
    }
}

}
