#include <kazbase/os.h>
#include "directory_watcher.h"
#include <iostream>

namespace delimit {

DirectoryWatcher::DirectoryWatcher(const unicode &root) {

    //Add a watcher for the directory
    add_watcher(root);
}

void DirectoryWatcher::add_watcher(const unicode& path) {
    std::cout << "Watching: " << path << std::endl;

    unicode abs = os::path::abs_path(path);

    auto file = Gio::File::create_for_path(abs.encode());
    if(!file->query_exists()) {
        return;
    }

    if(!os::path::is_dir(abs)) {
        return;
    }

    if(monitors_.find(abs) != monitors_.end()) {
        return;
    }

    monitors_[abs] = file->monitor_directory(Gio::FILE_MONITOR_SEND_MOVED);

    // Trigger added signals
    directory_created_(abs);
    for(auto f: os::path::list_dir(abs)) {
        auto fp = os::path::abs_path(os::path::join(abs, f));
        if(os::path::is_file(fp)) {
            file_created_(fp);
        }
    }

    // Now recursively add monitors for all folders down the tree
    for(auto file_or_dir: os::path::list_dir(abs)) {
        auto full = os::path::join(abs, file_or_dir);
        if(os::path::is_dir(full) && !full.ends_with(".") && !file_or_dir.starts_with(".")) {
            add_watcher(full);
        }
    }
}

void DirectoryWatcher::remove_watcher(const unicode& path) {
    auto start = monitors_.find(path);

    for(; start != monitors_.end();) {
        // If we hit an element that doesn't start with the path
        // then bail (the map should be sorted)
        if(!(*start).first.starts_with(path)) {
            break;
        }

        // Trigger signals for removal
        for(auto f: os::path::list_dir((*start).first)) {
            auto fp = os::path::abs_path(os::path::join((*start).first, f));
            if(os::path::is_file(fp)) {
                file_removed_(fp);
            }
        }
        directory_removed_((*start).first);

        start = monitors_.erase(start); //We erase everything until we break
    }
}


void DirectoryWatcher::on_directory_changed(const Glib::RefPtr<Gio::File>& file, const Glib::RefPtr<Gio::File>& other_file, Gio::FileMonitorEvent type) {
    switch(type) {
        case Gio::FILE_MONITOR_EVENT_CREATED: {
            if(os::path::is_dir(file->get_path())) {
                add_watcher(file->get_path()); //Directory created so add a watcher
            } else {
                file_created_(file->get_path());
            }
        }
        break;
        case Gio::FILE_MONITOR_EVENT_DELETED: {
            if(os::path::is_dir(file->get_path())) {
                remove_watcher(file->get_path());
            } else {
                file_removed_(file->get_path());
            }
        }
        break;
        case Gio::FILE_MONITOR_EVENT_MOVED: {
            if(os::path::is_dir(file->get_path())) {
                remove_watcher(file->get_path()); //Directory removed so delete watcher
                add_watcher(other_file->get_path()); // Add the new directory
            } else {
                file_removed_(file->get_path());
                file_created_(other_file->get_path());
            }
        }
        break;
        default:
            break;
    }
}

}
