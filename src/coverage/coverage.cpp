#include <iostream>

#include "coverage.h"
#include "../window.h"
#include "../utils.h"
#include "../document_view.h"

using namespace coverage;

void Coverage::clear_document(delimit::DocumentView* buffer) {
    auto gbuf = buffer->buffer();

    gbuf->remove_source_marks(gbuf->begin(), gbuf->end(), Glib::ustring("coverage"));
}

void Coverage::apply_to_document(delimit::DocumentView *buffer) {
    auto gbuf = buffer->buffer();
    clear_document(buffer);

    auto result = find_uncovered_lines(buffer->path(), buffer->window().project_path());
    for(auto line: result) {
        if(!(line >= 0 && line < gbuf->end().get_line())) {
            continue;
        }

        auto iter = gbuf->get_iter_at_line(line);
        assert(iter.get_line() == line);

        auto mark = gbuf->create_source_mark(
            Glib::ustring("coverage"), iter
        );

        mark->set_visible(true);
    }

    std::cout << "Done adding marks" <<std::endl;
}

unicode PythonCoverage::find_coverage_file(const unicode& filename, const unicode& project_root) {
    //Search up to the project root, to find the coverage file
    unicode current_dir = os::path::abs_path(os::path::dir_name(filename));
    while(current_dir != project_root) {
        unicode coverage = os::path::join(current_dir, ".coverage");
        if(os::path::exists(coverage)) {
            break;
        }

        unicode next_level = os::path::abs_path(os::path::dir_name(current_dir));
        if(next_level == current_dir) {
            break;
        }

        current_dir = next_level;
    }


    unicode final = os::path::join(current_dir, ".coverage");
    if(os::path::exists(final)) {
        return final;
    } else {
        return "";
    }
}

std::vector<int32_t> PythonCoverage::find_uncovered_lines(const unicode &filename, const unicode &project_root) {
    unicode coverage_file = find_coverage_file(filename, project_root);
    if(coverage_file.empty()) {
        return std::vector<int32_t>();
    }

    if(coverage_monitors_.find(coverage_file) == coverage_monitors_.end()) {
        // If this is a coverage file we've never seen before, then start watching!
        auto file = Gio::File::create_for_path(coverage_file.encode());
        auto monitor = file->monitor_file();
        monitor->signal_changed().connect([&](const Glib::RefPtr<Gio::File>&, const Glib::RefPtr<Gio::File>&, Gio::FileMonitorEvent evt) {
            if(evt == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT || evt == Gio::FILE_MONITOR_EVENT_CREATED) {
                signal_coverage_updated_();
            } else if(evt == Gio::FILE_MONITOR_EVENT_DELETED) {
                coverage_monitors_.erase(coverage_file);
            }
        });
        coverage_monitors_[coverage_file] = monitor;
    }

    unicode current_dir = os::path::dir_name(coverage_file);

    unicode coverage_command;

    for(auto command: { "python-coverage", "coverage"}) {
        coverage_command = call_command(_u("which {0}").format(command));
        if(!coverage_command.empty()) {
            break;
        }
    }

    if(coverage_command.empty()) {
        return std::vector<int32_t>();
    }

    unicode result = call_command(
        _u("{0} report -m --include {1}").format(coverage_command, filename),
        current_dir
    );

    unicode last_line = result.split("\n").back();
    unicode missing = last_line.split("  ").back();
    std::cout << missing << std::endl;
    std::vector<unicode> batches = missing.split(",");

    std::vector<int32_t> ret;
    for(unicode batch: batches) {
        batch = batch.strip();

        auto high_low = batch.split("-");
        if(high_low.size() == 1) {
            try {
                ret.push_back(high_low.front().to_int() - 1);
            } catch(std::exception& e) {
                continue;
            }
        } else {
            try {
                int start = high_low.front().to_int() - 1;
                int end = high_low.back().to_int() - 1;
                for(int i = start; i <= end; ++i) {
                    ret.push_back(i);
                }
            } catch(std::exception& e) {
                continue;
            }
        }
    }
    return ret;
}
