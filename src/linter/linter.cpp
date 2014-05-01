#include "linter.h"

#include "../frame.h"
#include "../buffer.h"
#include "../window.h"
#include "../utils.h"

using namespace linter;

void Linter::clear_buffer(delimit::Buffer* buffer) {
    auto gbuf = buffer->_gtk_buffer();

    gbuf->remove_source_marks(gbuf->begin(), gbuf->end(), Glib::ustring("linter"));
}

void Linter::apply_to_buffer(delimit::Buffer *buffer) {
    auto gbuf = buffer->_gtk_buffer();
    clear_buffer(buffer);

    auto result = find_problematic_lines(buffer->path(), buffer->window().project_path());
    for(auto line_and_message: result) {
        auto line = line_and_message.first;
        auto message = line_and_message.second;

        assert(line >= 0 && line < gbuf->end().get_line());

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

    buffer->window().set_error_count(result.size());
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
            unicode message = line.split(":")[2];
            int32_t lineno = line.split(":")[1].to_int() - 1;
            ret.push_back(std::make_pair(lineno, message));

            std::cout << lineno << ": " << message << std::endl;
        } catch(boost::bad_lexical_cast& e) {
            continue;
        }
    }

    return ret;
}
