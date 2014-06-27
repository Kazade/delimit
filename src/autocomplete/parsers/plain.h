#ifndef PLAIN_H
#define PLAIN_H

#include "../base.h"

namespace delimit {
namespace parser {

class Plain : public delimit::FileParser {
public:
    std::pair<std::vector<ScopePtr>, bool> parse(const unicode& data, const unicode& base_scope);
    unicode base_scope_from_filename(const unicode &filename);
    std::vector<unicode> tokenize(const unicode& data);
};

}
}
#endif // PLAIN_H
