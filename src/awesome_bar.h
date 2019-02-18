#ifndef AWESOME_BAR_H
#define AWESOME_BAR_H

#include <gtkmm.h>
#include <future>

#include "utils/unicode.h"

namespace delimit {

class Window;

class AwesomeBar : public Gtk::VBox {
public:
    AwesomeBar(Window& parent);

    void show() {
        entry_.set_text("");
        entry_.grab_focus();
        Gtk::VBox::show();
    }

    void repopulate_files();
private:
    Window& window_;

    Gtk::Entry entry_;
    Gtk::Revealer list_revealer_;
    Gtk::ListBox list_;
    Gtk::ScrolledWindow list_window_;

    void build_widgets();

    std::vector<unicode> filter_project_files(const unicode& search_text, uint64_t filter_task_id);
    void populate_results(const std::vector<unicode>& to_add);

    void populate(const unicode& text);
    void execute();

    std::vector<unicode> project_files_;
    std::vector<unicode> displayed_files_;

    uint64_t filter_task_id_ = 0;
    std::shared_ptr<std::future<std::vector<unicode>>> filter_task_;
    std::mutex filter_cache_lock;
};

}

#endif // AWESOME_BAR_H
