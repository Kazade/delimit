#ifndef AWESOME_BAR_H
#define AWESOME_BAR_H

#include <gtkmm.h>

namespace delimit {

class Frame;

class AwesomeBar : public Gtk::VBox {
public:
    AwesomeBar(Frame* parent);

    void show() {
        entry_.grab_focus();
        Gtk::VBox::show();
    }

private:
    Frame* frame_;

    Gtk::Entry entry_;

    void build_widgets();

    void populate(const unicode& text);
    void execute();
};

}

#endif // AWESOME_BAR_H
