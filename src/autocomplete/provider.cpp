#include <iostream>
#include "provider.h"
#include "datastore.h"

#include "../window.h"

#include "parsers/plain.h"
#include "parsers/python.h"

namespace delimit {

Provider::Provider(Window* window):
    Glib::ObjectBase(typeid(Provider)),
    Glib::Object(),
    Gsv::CompletionProvider() {
    indexer_ = std::make_shared<Indexer>("/tmp/test.db");

    indexer_->register_parser("text/plain", std::make_shared<parser::Plain>());
    indexer_->register_parser("application/python", std::make_shared<parser::Python>());

    ///Just a test, let's try and index the tokenize.py library

    indexer_->index_file("/usr/lib64/python2.7/os.py");
}

void Provider::populate_vfunc(const Glib::RefPtr<Gsv::CompletionContext> &context) {
    unicode allowed_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";

    auto iter = context->get_iter();
    auto buffer = iter.get_buffer();
    int line = iter.get_line();
    int col = iter.get_line_offset();

    auto start = iter;

    while(start.backward_char()) {
        unicode ch = unicode(1, (char) start.get_char());
        if(!allowed_chars.contains(ch) && ch != ".") {
            start.forward_char();
            break;
        }
    }

    auto incomplete = buffer->get_text(start, iter, true);
    if(incomplete.empty()) {
        return;
    }

    //FIXME: Determine the line that needs completing, and the current filename
    std::cout << "Populate called: " << incomplete << std::endl;
    auto completions = indexer_->datastore()->query_completions("", line, col, unicode(incomplete.c_str()));

    /*
    Glib::RefPtr<Provider> reffed_this(this);
    reffed_this->reference();

    context->add_proposals(reffed_this, {Gsv::CompletionItem::create("Test", "Test", Glib::RefPtr<Gdk::Pixbuf>(), "") }, true);
    */
}

Gsv::CompletionActivation Provider::get_activation_vfunc() const {
    return Gsv::COMPLETION_ACTIVATION_INTERACTIVE | Gsv::COMPLETION_ACTIVATION_USER_REQUESTED;
}

}
