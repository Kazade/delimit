#include <gtkmm.h>
#include <gtksourceviewmm.h>

#include "window.h"
#include "base/os.h"
#include "base/logging.h"
#include "base/fdo/base_directory.h"
#include "utils/sigc_lambda.h"

namespace delimit {

class Application: public Gtk::Application {
protected:
    explicit Application(int& argc, char**& argv, const Glib::ustring& application_id = Glib::ustring(), Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE):
        Gtk::Application(argc, argv, application_id, flags) {

        logging::get_logger("/")->add_handler(logging::Handler::ptr(new logging::StdIOHandler));

        signal_startup().connect(sigc::mem_fun(this, &Application::on_signal_startup));
        signal_open().connect(sigc::mem_fun(this, &Application::on_signal_open));

        for(int i = 0; i < argc; ++i) {
            args_.push_back(argv[i]);
        }

        Gsv::init();
    }

private:
    std::vector<std::string> args_;

public:
    using Gtk::Application::add_window;

    static Glib::RefPtr<Application> create(
            int& argc, char**& argv,
            const Glib::ustring& application_id = Glib::ustring(),
            Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE) {

        return Glib::RefPtr<delimit::Application>(new delimit::Application(argc, argv, application_id, flags));
    }

    void add_window(Window::ptr window) {
        windows_.push_back(window);
        add_window(window->_gtk_window());
        window->_gtk_window().show_all();
    }

    void on_signal_open(const Gio::Application::type_vec_files& files, const Glib::ustring& line) {
        L_INFO(_u("on_signal_open {0}").format(line));

        if(files.empty()) {
            return;
        }

        auto window = std::make_shared<delimit::Window>(files);
        add_window(window);
    }

    void on_signal_startup() {
        L_INFO(_u("on_signal_startup"));

        //Add our schemes to the list of available ones
        auto manager = Gsv::StyleSchemeManager::get_default();
        std::string ui_file = fdo::xdg::find_data_file("delimit/schemes").encode();
        manager->prepend_search_path(ui_file);
    }

private:
    std::vector<Window::ptr> windows_;
};

}

int main(int argc, char* argv[]) {
    Glib::RefPtr<Gtk::Application> app =
        delimit::Application::create(argc, argv, "uk.co.kazade.Delimit",
            Gio::APPLICATION_HANDLES_OPEN
        );

    assert(app);

    return app->run();
}
