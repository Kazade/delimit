#ifndef INDEXER_H
#define INDEXER_H

#include <map>

#include "base.h"

namespace delimit {

class Indexer {
public:
    Indexer(const unicode& path_to_datastore);

    void register_parser(const unicode& mimetype, FileParserPtr parser);
    void index_directory(const unicode& dir_path);
    std::vector<ScopePtr> index_file(const unicode& path);
    std::vector<ScopePtr> index_file(const unicode& filename, const unicode& data);

    void clear(); //Wipe out the save index file

    DatastorePtr datastore() { return datastore_; }

    FileParserPtr parser(const unicode& name) { return parsers_by_name_.at(name); }
private:
    FileParserPtr detect_parser(const unicode& filename);
    unicode guess_type(const unicode& filename);

    std::map<unicode, FileParserPtr> parsers_;
    std::map<unicode, FileParserPtr> parsers_by_name_;
    DatastorePtr datastore_;
};

}

#endif // INDEXER_H
