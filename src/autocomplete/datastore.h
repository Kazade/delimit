#ifndef DATASTORE_H
#define DATASTORE_H

#include <sqlite3.h>

#include "./base.h"

namespace delimit {

class Datastore {
public:
    Datastore(const unicode& path_to_datastore);

    void delete_scopes_by_filename(const unicode& path);
    void save_scopes(const unicode& parser_name, const std::vector<ScopePtr>& scopes, const unicode& filename);

    std::vector<unicode> query_completions(const unicode &parser,
        const unicode& filename,
        int line_number,
        int col_number,
        const unicode& string_to_complete
    , bool word_completion_only=false);

    unicode query_scope_at(const unicode &parser, const unicode& filename, int line_number, int col_number);
private:
    sqlite3* db_;

    void initialize_tables();
    void open_and_recreate_if_necessary(const unicode& path_to_datastore);

    unicode query_database_version();
};


}

#endif // DATASTORE_H
