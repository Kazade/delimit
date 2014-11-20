#ifndef DIRECTORY_WATCHER_H
#define DIRECTORY_WATCHER_H

#include <map>
#include <mutex>
#include <future>

#include <gtkmm.h>
#include <kazbase/signals.h>
#include <kazbase/unicode.h>

namespace delimit {

class DirectoryWatcher {
public:
    DirectoryWatcher(const unicode& root);

    sig::signal<void (unicode)>& signal_file_created() { return file_created_; }
    sig::signal<void (unicode)>& signal_file_removed() { return file_removed_; }
    sig::signal<void (unicode)>& signal_directory_created() { return directory_created_; }
    sig::signal<void (unicode)>& signal_directory_removed() { return directory_removed_; }

    bool is_dead() const { return monitors_.empty(); }

private:
    std::mutex monitor_mutex_;

    sig::signal<void (unicode)> file_created_;
    sig::signal<void (unicode)> file_removed_;
    sig::signal<void (unicode)> directory_created_;
    sig::signal<void (unicode)> directory_removed_;

    std::map<unicode, Glib::RefPtr<Gio::FileMonitor> > monitors_;

    void add_watcher_in_background(const unicode& path);
    void add_watcher(const unicode& path);
    bool do_add_watcher(const unicode& path);
    void remove_watcher(const unicode& path);

    void on_directory_changed(const Glib::RefPtr<Gio::File>& file, const Glib::RefPtr<Gio::File>& other_file, Gio::FileMonitorEvent type);
};

}

#endif // DIRECTORY_WATCHER_H
