#ifndef PROVIDER_H
#define PROVIDER_H

#include <gtksourceviewmm.h>

#include "indexer.h"

namespace delimit {

class Window;

class Provider : public Gsv::CompletionProvider {
public:
    Provider(Window *window);

    Glib::ustring get_name() const { return "Delimit"; }
    void populate(const Glib::RefPtr<Gsv::CompletionContext> &context);

private:
    Window* window_;
    IndexerPtr indexer_;
};

}

#endif // PROVIDER_H
