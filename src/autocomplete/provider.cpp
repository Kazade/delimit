#include <iostream>
#include "provider.h"
#include "datastore.h"

#include "../window.h"

#include "parsers/plain.h"
#include "parsers/python.h"

#include <kazbase/hash/md5.h>
#include <kazbase/os.h>
#include <kazbase/fdo/base_directory.h>

namespace delimit {

unicode get_parser_to_use(const unicode& mime) {
    return "PLAIN";
}

Provider::Provider(Window* window):
    Gsv::CompletionProvider(),
    Glib::ObjectBase(typeid(Provider)),
    Glib::Object() {

    unicode folder = fdo::xdg::make_dir_in_data_home("delimit");
    unicode database_file = os::path::join(folder, "completions.db");

    indexer_ = std::make_shared<Indexer>(database_file);

    indexer_->register_parser("text/plain", std::make_shared<parser::Plain>());
    indexer_->register_parser("application/python", std::make_shared<parser::Python>());
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
    auto completions = indexer_->datastore()->query_completions(
        get_parser_to_use("FIXME"), "", line, col, unicode(incomplete.c_str())
    );


    std::vector<Glib::RefPtr<Gsv::CompletionProposal>> results;

    Glib::RefPtr<Gdk::Pixbuf> icon = Gtk::IconTheme::get_default()->load_icon("info", Gtk::ICON_SIZE_MENU);

    for(auto completion: completions) {
        results.push_back(Gsv::CompletionItem::create(completion.encode(), completion.encode(), icon, ""));
    }

    if(results.empty()) {
        return;
    }

    Glib::RefPtr<Provider> reffed_this(this);
    reffed_this->reference();

    context->add_proposals(reffed_this, results, true);
}

Gsv::CompletionActivation Provider::get_activation_vfunc() const {
    return Gsv::COMPLETION_ACTIVATION_INTERACTIVE | Gsv::COMPLETION_ACTIVATION_USER_REQUESTED;
}

}
