#ifndef AWESOME_BAR_H
#define AWESOME_BAR_H

#include <gtkmm.h>

namespace delimit {

class Window;

class AwesomeBar : public Gtk::VBox {
public:
    AwesomeBar(Window& parent);

    void show() {
        entry_.grab_focus();
        Gtk::VBox::show();
    }

    void repopulate_files();
private:
    Window& window_;

    Gtk::Entry entry_;
    Gtk::Revealer list_revealer_;
    Gtk::ListBox list_;

    void build_widgets();

    void populate(const unicode& text);
    void execute();

    std::vector<unicode> project_files_;
};

}

#endif // AWESOME_BAR_H
