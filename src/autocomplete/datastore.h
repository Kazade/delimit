#ifndef DATASTORE_H
#define DATASTORE_H

#include <sqlite3.h>

namespace delimit {

class Datastore {
public:
    Datastore(const unicode& path_to_datastore);

    void delete_scopes_by_filename(const unicode& path);
    void save_scopes(const unicode& filename, const std::vector<ScopePtr>& scopes);

private:
    sqlite3* db_;

    void initialize_tables();
};


}

#endif // DATASTORE_H
