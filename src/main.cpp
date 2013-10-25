#include <gtkmm.h>

#include "window.h"
#include "base/os.h"
#include "base/logging.h"
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

        if(files.size() > 1) {
            //If we have more than one file, open them in the same window
            auto window = std::make_shared<delimit::Window>(delimit::WINDOW_TYPE_FILE, files[0]->get_path());
            add_window(window);

            for(int i = 1; i < files.size(); ++i) {
                window->open_buffer(files[i]->get_path());
            }
        } else {
            //Otherwise pass through to start up and handle a single file/folder
            _start_up(files[0]->get_path());
        }
    }

    void on_signal_startup() {
        L_INFO(_u("on_signal_startup"));
    }

private:
    void _start_up(const unicode& path) {
        if(path.empty()) {
            //Open a new blank buffer
            auto window = std::make_shared<delimit::Window>();
            add_window(window);
        } else {
            if(os::path::is_file(path)) {
                auto window = std::make_shared<delimit::Window>(delimit::WINDOW_TYPE_FILE, path);
                add_window(window);
            } else {
                L_INFO(_u("Opening window in folder mode for path {0}").format(path));
                auto window = std::make_shared<delimit::Window>(delimit::WINDOW_TYPE_FOLDER, path);
                add_window(window);
            }
        }
    }

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
