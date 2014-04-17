
#include "coverage.h"
#include "../frame.h"
#include "../buffer.h"
#include "../window.h"

using namespace coverage;

unicode call_command(unicode command, unicode cwd="") {
    char tmpname [L_tmpnam];
    std::tmpnam ( tmpname );
    std::string scommand = command.encode();
    if(!cwd.empty()) {
        scommand = _u("cd {0}; {1}").format(cwd, scommand).encode();
    }
    std::string cmd = scommand + " >> " + tmpname;
    std::system(cmd.c_str());
    std::ifstream file(tmpname, std::ios::in );
    std::vector<unicode> lines;
    while(file.good()) {
        std::string line;
        std::getline(file, line);
        lines.push_back(line);
    }

    file.close();

    remove(tmpname);
    return _u("\n").join(lines);
}

void Coverage::clear_buffer(delimit::Buffer* buffer) {
    auto gbuf = buffer->_gtk_buffer();

    gbuf->remove_source_marks(gbuf->begin(), gbuf->end(), "error");
}

void Coverage::apply_to_buffer(delimit::Buffer *buffer) {
    auto gbuf = buffer->_gtk_buffer();
    clear_buffer(buffer);

    auto result = find_uncovered_lines(buffer->path(), buffer->window().project_path());
    for(auto line: result) {
        auto iter = gbuf->get_iter_at_line(line);
        gbuf->create_source_mark(
            "error", iter
        );
    }

    std::cout << "Done adding marks" <<std::endl;
}

std::vector<int32_t> PythonCoverage::find_uncovered_lines(const unicode &filename, const unicode &project_root) {
    //Return nothing if we can't find a coverage file in the project root
    if(!os::path::exists(os::path::join(project_root, ".coverage"))) {
        return std::vector<int32_t>();
    }

    unicode result = call_command(
        _u("python-coverage report -m --include {0}").format(filename),
        project_root
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
            ret.push_back(high_low.front().to_int());
        } else {
            int start = high_low.front().to_int();
            int end = high_low.back().to_int();
            for(int i = start; i <= end; ++i) {
                ret.push_back(i);
            }
        }
    }
    return ret;
}
