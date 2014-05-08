#ifndef PROVIDER_H
#define PROVIDER_H

#include <gtksourceviewmm.h>

namespace delimit {

class Provider : public Gsv::CompletionProvider {
public:
    Glib::ustring get_name() const { return "Delimit"; }
    void populate(const Glib::RefPtr<Gsv::CompletionContext> &context);
};

}

#endif // PROVIDER_H
