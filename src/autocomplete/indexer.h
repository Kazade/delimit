#ifndef INDEXER_H
#define INDEXER_H

#include "base.h"

namespace delimit {

class Indexer {
public:
    Indexer(const unicode& path_to_datastore);

    void register_parser(const unicode& mimetype, FileParserPtr parser);
    void index_directory(const unicode& dir_path);
    std::vector<ScopePtr> index_file(const unicode& path);

    void clear(); //Wipe out the save index file

private:
    FileParserPtr detect_parser(const unicode& filename);
    unicode guess_type(const unicode& filename);

    std::map<unicode, FileParserPtr> parsers_;
    DatastorePtr datastore_;
};

}

#endif // INDEXER_H
