#include <type_traits>
#include <sigc++/sigc++.h>
#include <kazbase/os.h>
#include <kazbase/logging.h>

#include "directory_watcher.h"
#include <iostream>
#include "utils.h"

namespace sigc
{
    template <typename Functor>
    struct functor_trait<Functor, false>
    {
        typedef decltype (::sigc::mem_fun (std::declval<Functor&> (),
                                           &Functor::operator())) _intermediate;

        typedef typename _intermediate::result_type result_type;
        typedef Functor functor_type;
    };
}

namespace delimit {

DirectoryWatcher::DirectoryWatcher(const unicode &root) {

    //Add a watcher for the directory
    add_watcher_in_background(root);
}

void DirectoryWatcher::add_watcher_in_background(const unicode& path) {
    struct Wrapper {
        std::future<bool> future;
    };

    auto wrapper = std::make_shared<Wrapper>();
    wrapper->future = std::async(std::launch::async, std::bind(&DirectoryWatcher::do_add_watcher, this, path));

    Glib::signal_idle().connect([=]() -> bool {
        if(wrapper->future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) {
            return false;
        }

        return true;
    });
}

void DirectoryWatcher::add_watcher(const unicode& path) {
    do_add_watcher(path);
}

bool DirectoryWatcher::do_add_watcher(const unicode& path) {
    std::cout << "Watching: " << path << std::endl;

    unicode abs = os::path::abs_path(path);

    auto file = Gio::File::create_for_path(abs.encode());
    if(!file->query_exists()) {
        return false;
    }

    if(!os::path::is_dir(abs)) {
        return false;
    }

    {
        // Lock while we check/update the monitors list
        std::lock_guard<std::mutex> lock(monitor_mutex_);
        if(monitors_.find(abs) != monitors_.end()) {
            return false;
        }

        monitors_[abs] = file->monitor_directory(Gio::FILE_MONITOR_SEND_MOVED);
    }

    // Trigger added signals
    try {
        directory_created_(abs);
    } catch(...) {
        // We never want a signal to kill the recursive process
        L_ERROR("An error was detected while calling the file created signal");
    }

    for(auto f: os::path::list_dir(abs)) {
        auto fp = os::path::abs_path(os::path::join(abs, f));
        if(os::path::is_file(fp)) {
            try {
                file_created_(fp);
            } catch(...) {
                // We never want a signal to kill the recursive process
                L_ERROR("An error was detected while calling the file created signal");
            }
        }
    }

    // Now recursively add monitors for all folders down the tree
    for(auto file_or_dir: os::path::list_dir(abs)) {
        auto full = os::path::join(abs, file_or_dir);
        if(os::path::is_dir(full) && !full.ends_with(".") && !file_or_dir.starts_with(".")) {
            do_add_watcher(full);
        }
    }

    return true;
}

void DirectoryWatcher::remove_watcher(const unicode& path) {
    std::lock_guard<std::mutex> lock(monitor_mutex_);

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
                add_watcher_in_background(file->get_path()); //Directory created so add a watcher
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
                add_watcher_in_background(other_file->get_path()); // Add the new directory
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
