#ifndef APPLICATION_H
#define APPLICATION_H

#include <gtkmm.h>
#include <memory>

#include "utils/unicode.h"

namespace delimit {

void clear_tempfiles_directory();
unicode tempfiles_directory();

class Window;

class Application: public Gtk::Application {
protected:
    explicit Application(int& argc, char**& argv, const Glib::ustring& application_id = Glib::ustring(), Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE);
private:
    std::vector<std::string> args_;

public:
    using Gtk::Application::add_window;

    static Glib::RefPtr<Application> create(
            int& argc, char**& argv,
            const Glib::ustring& application_id = Glib::ustring(),
            Gio::ApplicationFlags flags = Gio::APPLICATION_FLAGS_NONE);

    void add_window(std::shared_ptr<Window> window);
    void on_signal_open(const Gio::Application::type_vec_files& files, const Glib::ustring& line);
    void on_signal_startup();

protected:
    virtual void on_startup();

private:

    void action_open_folder(const Glib::VariantBase&);
    void action_open_file(const Glib::VariantBase&);
    void action_quit(const Glib::VariantBase&);

    std::vector<std::shared_ptr<Window>> windows_;

    Glib::RefPtr<Gio::Menu> app_menu_;
    Glib::RefPtr<Gio::SimpleAction> action_open_folder_;

};


}

#endif // APPLICATION_H
