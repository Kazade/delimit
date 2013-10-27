#include "base/logging.h"
#include "base/fdo/base_directory.h"

#include "application.h"
#include "window.h"

namespace delimit {

Application::Application(int& argc, char**& argv, const Glib::ustring& application_id, Gio::ApplicationFlags flags):
    Gtk::Application(argc, argv, application_id, flags) {

    logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

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
    L_INFO(_u("on_signal_open {0}").format(line));

    if(files.empty()) {
        return;
    }

    auto window = std::make_shared<delimit::Window>(files);
    add_window(window);
}

void Application::on_signal_startup() {
    L_INFO(_u("on_signal_startup"));

    //Add our schemes to the list of available ones
    auto manager = Gsv::StyleSchemeManager::get_default();
    std::string ui_file = fdo::xdg::find_data_file("delimit/schemes").encode();
    manager->prepend_search_path(ui_file);

    if(args_.size() == 1) {
        auto window = std::make_shared<delimit::Window>();
        add_window(window);
    }
}
}
