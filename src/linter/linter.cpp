#include <iostream>
#include "linter.h"

#include "../frame.h"
#include "../buffer.h"
#include "../window.h"
#include "../utils.h"

using namespace linter;

void Linter::clear_document(delimit::DocumentView* buffer) {
    auto gbuf = buffer->buffer();

    gbuf->remove_source_marks(gbuf->begin(), gbuf->end(), Glib::ustring("linter"));
}

void Linter::apply_to_document(delimit::DocumentView *buffer) {
    auto gbuf = buffer->buffer();
    clear_document(buffer);

    auto result = find_problematic_lines(buffer->path(), buffer->window().project_path());
    for(auto line_and_message: result) {
        auto line = line_and_message.first;
        auto message = line_and_message.second;

        if(line < 0 || line > buffer->buffer()->end().get_line()) {
            continue;
        }

        auto iter = gbuf->get_iter_at_line(line);
        assert(iter.get_line() == line);

        auto mark = gbuf->create_source_mark(
            Glib::ustring("linter"), iter
        );

        //Associate the message with the mark
        mark->set_data(
            "linter_message",
            new unicode(message),
            [=](void* data) {
                delete (unicode*) data;
            }
        );

        mark->set_visible(true);
    }

    buffer->set_error_count(result.size());
    buffer->window().set_error_count(buffer->error_count());
}

LinterResult PythonLinter::find_problematic_lines(const unicode& filename, const unicode& project_root) {
    unicode result = call_command(
        _u("pyflakes {0}").format(filename)
    );

    if(result.strip().empty()) {
        return LinterResult();
    }

    LinterResult ret;

    for(auto line: result.split("\n")) {
        try {
            auto parts = line.split(":");
            unicode message = parts[parts.size()-1];
            int32_t lineno = parts[1].to_int() - 1;
            ret.push_back(std::make_pair(lineno, message));

            std::cout << lineno << ": " << message << std::endl;
        } catch(std::exception& e) {
            continue;
        }
    }

    return ret;
}
