#ifndef BFS_H
#define BFS_H

#include <gtkmm.h>

#include <queue>
#include <algorithm>
#include <kazbase/unicode.h>
#include <kazbase/os.h>
#include <kazbase/signals.h>

class BFS {
public:
    BFS(const unicode& root_path):
        root_(root_path) {

    }

    std::vector<unicode> run() {
        temp_.push(root_);

        std::vector<unicode> result;
        process_level(result, 0);
        return result;
    }

    sig::signal<void (const std::vector<unicode>&, int)>& signal_level_complete() { return signal_level_complete_; }

private:
    void process_level(std::vector<unicode>& result, int level_num) {
        if(temp_.empty()) {
            return;
        }

        auto level = temp_; //Copy the level
        temp_ = std::queue<unicode>();

        while(!level.empty()) {
            unicode file_or_folder = level.front();
            level.pop();

            if(file_or_folder.starts_with(".") || file_or_folder.ends_with(".pyc")) {
                continue;
            }

            unicode abs_path;
            try {
                abs_path = os::path::real_path(file_or_folder);
            } catch(...) {
                std::cout << "Unable to deal with file path " << std::endl;
                continue;
            }

            if(os::path::is_dir(abs_path)) {
                auto files = os::path::list_dir(abs_path);
                for(auto file: files) {
                    if(!file.starts_with(".")) {
                        temp_.push(os::path::join(file_or_folder, file));
                    }
                }
            }

            auto content_type = Gio::File::create_for_path(file_or_folder.encode())->query_info(
                G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE
            )->get_content_type();

            if(!_u(content_type).starts_with("text/") && !_u(content_type).starts_with("application/")) {
                //Only include text files
                continue;
            }

            result.push_back(file_or_folder);
        }

        signal_level_complete_(result, level_num);
        process_level(result, level_num + 1);
    }

    unicode root_;
    std::queue<unicode> temp_;

    sig::signal<void (const std::vector<unicode>&, int)> signal_level_complete_;
};

#endif // BFS_H

