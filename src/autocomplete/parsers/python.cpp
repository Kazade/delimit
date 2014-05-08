
#include "python.h"

namespace delimit {
namespace parser {

std::vector<Token> Python::tokenize(const unicode& data) {

}

std::vector<ScopePtr> Python::parse(const unicode& data)  {
    auto tokens = tokenize(data);

    //TODO: Build scopes
}

}
}
