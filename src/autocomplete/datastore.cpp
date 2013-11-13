#include "../base/unicode.h"

#include "datastore.h"

namespace delimit {

std::vector<unicode> INITIAL_DATA_SQL = {
    "BEGIN",
    "CREATE TABLE scope(id INTEGER PRIMARY KEY, filename VARCHAR(255) NOT NULL, path VARCHAR(255) NOT NULL)",
    "CREATE INDEX filename_idx ON scope(path)",
    "CREATE TABLE scope_parent(id INTEGER PRIMARY KEY, scope INTEGER, FOREIGN KEY(scope) REFERENCES scope(id))",
    "COMMIT",
};

Datastore::Datastore(const unicode &path_to_datastore):
    db_(nullptr) {

    bool create_tables = !os::path::exists(path_to_datastore);

    int rc = sqlite3_open(path_to_datastore.encode(), &db_);
    if(rc) {
        sqlite3_close(db_);
        throw IOError("Unable to create database");
    }

    if(create_tables) {
        initialize_tables();
    }
}

void Datastore::initialize_tables() {
    std::string final_statement = _u("; ").join(INITIAL_DATA_SQL).encode();
    int ret = sqlite3_exec(db_, final_statement.c_str(), 0, 0, 0);
    if(ret) {
        throw IOError("Unable to create database tables");
    }
}

void Datastore::delete_scopes_by_filename(const unicode& path) {
    unicode SQL = "DELETE FROM scope WHERE filename = ?";

    sqlite3_stmt* stmt;
    int ret = sqlite3_prepare(
        db_,
        SQL,
        -1,
        &stmt,
        0
    );

    assert(!ret);

    sqlite3_bind_text(stmt, 1, )

}

void Datastore::save_scopes(const unicode& filename, const std::vector<ScopePtr>& scopes) {

}


}
