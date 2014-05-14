#include "base.h"

namespace delimit {


Scope::Scope(const unicode& path, std::vector<unicode> inherited_scopes):
    path_(path),
    inherited_paths_(inherited_scopes) {


}

}
