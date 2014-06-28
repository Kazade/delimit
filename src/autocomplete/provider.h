#ifndef PROVIDER_H
#define PROVIDER_H

#include <gtksourceviewmm.h>

#include "indexer.h"

namespace delimit {

class Window;

class Provider :  public Gsv::CompletionProvider, public Glib::Object {
public:
    Provider(Window *window);

    Glib::ustring get_name() const { return "Delimit"; }

    void populate_vfunc(const Glib::RefPtr<Gsv::CompletionContext> &context) override;
    bool match_vfunc(const Glib::RefPtr<const Gsv::CompletionContext> &context) const override { return true; }
    Gsv::CompletionActivation get_activation_vfunc() const override;

    IndexerPtr indexer() { return indexer_; }
private:
    Window* window_;
    IndexerPtr indexer_;
};

}

#endif // PROVIDER_H
