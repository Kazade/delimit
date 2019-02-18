#include <cstdlib>
#include "base_directory.h"

#include "kfs.h"

namespace fdo {
namespace xdg {

static unicode get_user_home() {
    return kfs::path::expand_user("~");
}

static std::string get_env_var(const unicode& name) {
    char* env = getenv(name.encode().c_str());
    if(env) {
        return env;
    }

    return std::string();
}

unicode get_data_home() {
    using kfs::path::join;

    std::string home = get_env_var("XDG_DATA_HOME");
    if(!home.empty()) {
        return kfs::path::expand_user(home);
    }

    return join(get_user_home().encode(), join(".local", "share"));
}

/**
    Make a folder in the data home directory, creating the parent
    directories if they don't exist already. Return the full
    final path of the directory.
*/
unicode make_dir_in_data_home(const unicode& folder_name) {
    using kfs::path::join;
    unicode dest_path = get_data_home();

    //Make the data home folder if it doesn't exist
    //and make it hidden if necessary
    if(!kfs::path::exists(dest_path.encode())) {
        kfs::make_dirs(dest_path.encode());
        kfs::path::hide_dir(dest_path.encode());
    }

    unicode final_path = join(dest_path.encode(), folder_name.encode());
    kfs::make_dirs(final_path.encode());
    return final_path;
}

unicode get_config_home() {
    using kfs::path::join;

    auto home = get_env_var("XDG_CONFIG_HOME");
    if(!home.empty()) {
        return kfs::path::expand_user(home);
    }

    return join(get_user_home().encode(), ".config");
}

/**
    Make a folder in the config home directory, creating the parent
    directories if they don't exist already. Return the full
    final path of the directory.
*/
unicode make_dir_in_config_home(const unicode& folder_name) {
    using kfs::path::join;
    unicode dest_path = get_config_home();

    //Make the data home folder if it doesn't exist
    //and make it hidden if necessary
    if(!kfs::path::exists(dest_path.encode())) {
        kfs::make_dirs(dest_path.encode());
        kfs::path::hide_dir(dest_path.encode());
    }

    auto final_path = join(dest_path.encode(), folder_name.encode());
    kfs::make_dirs(final_path);
    return final_path;
}

unicode get_cache_home() {
    using kfs::path::join;

    auto home = get_env_var("XDG_CACHE_HOME");
    if(!home.empty()) {
        return kfs::path::expand_user(home);
    }

    return join(get_user_home().encode(), ".cache");
}

std::vector<unicode> get_data_dirs() {
    unicode data_dir_s = get_env_var("XDG_DATA_DIRS");
    if(data_dir_s.empty()) {
        data_dir_s = "/usr/local/share/:/usr/share/";
    }

    std::vector<unicode> data_dirs = data_dir_s.split(":");
    return data_dirs;
}

std::vector<unicode> get_config_dirs() {
    unicode data_dir_s = get_env_var("XDG_CONFIG_DIRS");
    if(data_dir_s.empty()) {
        data_dir_s = "/etc/xdg";
    }

    std::vector<unicode> data_dirs = data_dir_s.split(":");
    return data_dirs;
}

std::pair<unicode, bool> find_data_file(const unicode& relative_path) {
    std::vector<unicode> data_dirs = get_data_dirs();

    if(kfs::path::exists(relative_path.encode())) {
        return std::make_pair(kfs::path::abs_path(relative_path.encode()), true);
    }

    for(std::vector<unicode>::iterator it = data_dirs.begin(); it != data_dirs.end(); ++it) {
        auto final_path = kfs::path::join((*it).encode(), relative_path.encode());
        if(kfs::path::exists(final_path)) {
            return std::make_pair(unicode(final_path), true);
        }
    }

    return std::make_pair(unicode(), false);
}

std::pair<unicode, bool> find_config_file(const unicode& relative_path) {
    std::vector<unicode> config_dirs = get_config_dirs();
    for(std::vector<unicode>::iterator it = config_dirs.begin(); it != config_dirs.end(); ++it) {
        auto final = kfs::path::join((*it).encode(), relative_path.encode());
        if(kfs::path::exists(final)) {
            return std::make_pair(final, true);
        }
    }

    return std::make_pair(unicode(), false);
}


std::pair<unicode, bool> find_user_data_file(const unicode &relative_path) {
    unicode data_home = get_data_home();
    auto final = kfs::path::join(data_home.encode(), relative_path.encode());
    if(kfs::path::exists(final)) {
        return std::make_pair(final, true);
    }

    return std::make_pair(unicode(), false);
}

std::pair<unicode, bool> find_user_config_file(const unicode& relative_path) {
    unicode config_home = get_config_home();
    auto final = kfs::path::join(config_home.encode(), relative_path.encode());
    if(kfs::path::exists(final)) {
        return std::make_pair(final, true);
    }

    return std::make_pair(unicode(), false);
}

unicode get_or_create_program_cache_path(const unicode& program_name) {
    auto path = kfs::path::join(get_cache_home().encode(), program_name.encode());
    if(!kfs::path::exists(path)) {
        kfs::make_dirs(path);
    }
    return path;
}

}
}

