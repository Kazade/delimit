#include "base.h"

namespace delimit {


Scope::Scope(const unicode& path, std::vector<unicode> inherited_scopes):
    path_(path),
    inherited_paths_(inherited_scopes),
    start_line(0),
    end_line(0),
    start_col(0),
    end_col(0) {


}

}
