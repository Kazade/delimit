#include <iostream>

#include "coverage.h"
#include "../window.h"
#include "../utils.h"
#include "../document_view.h"
#include "../utils/kfs.h"

using namespace coverage;

void Coverage::clear_document(delimit::DocumentView* buffer) {
    auto gbuf = buffer->buffer();

    gbuf->remove_source_marks(gbuf->begin(), gbuf->end(), Glib::ustring("coverage"));
}

void Coverage::apply_to_document(delimit::DocumentView *buffer) {
    L_DEBUG("Applying coverage to buffer");

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
    unicode current_dir = kfs::path::abs_path(kfs::path::dir_name(filename.encode()));
    while(current_dir != project_root) {
        auto coverage = kfs::path::join(current_dir.encode(), ".coverage");
        if(kfs::path::exists(coverage)) {
            break;
        }

        unicode next_level = kfs::path::abs_path(kfs::path::dir_name(current_dir.encode()));
        if(next_level == current_dir) {
            break;
        }

        current_dir = next_level;
    }


    auto final = kfs::path::join(current_dir.encode(), ".coverage");
    if(kfs::path::exists(final)) {
        L_DEBUG("Found coverage file");
        return final;
    } else {
        return "";
    }
}

PythonCoverage::~PythonCoverage() {
    for(auto it: coverage_monitors_) {
        it.second->cancel();
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
        monitor->signal_changed().connect([=](const Glib::RefPtr<Gio::File>&, const Glib::RefPtr<Gio::File>&, Gio::FileMonitorEvent evt) {
            L_DEBUG("Coverage changed event received");
            if(evt == Gio::FILE_MONITOR_EVENT_CHANGES_DONE_HINT || evt == Gio::FILE_MONITOR_EVENT_CREATED) {
                L_DEBUG("Signaling coverage changed");
                this->signal_coverage_updated_();
            } else if(evt == Gio::FILE_MONITOR_EVENT_DELETED) {
                L_DEBUG("Coverage file delted, erasing monitor");
                this->coverage_monitors_.erase(coverage_file);
            }
        });
        coverage_monitors_[coverage_file] = monitor;
    }

    unicode current_dir = kfs::path::dir_name(coverage_file.encode());

    unicode coverage_command;

    for(auto command: { "python-coverage", "coverage"}) {
        coverage_command = call_command(_F("which {0}").format(command));
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
