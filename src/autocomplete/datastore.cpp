#include <kazbase/unicode.h>
#include <kazbase/os/path.h>
#include <kazbase/exceptions.h>

#include "datastore.h"

namespace delimit {

std::vector<unicode> INITIAL_DATA_SQL = {
    "BEGIN",
    "CREATE TABLE scope(id INTEGER PRIMARY KEY, filename VARCHAR(255) NOT NULL, path VARCHAR(1024) NOT NULL)",
    "CREATE INDEX filename_idx ON scope(path)",
    "CREATE TABLE scope_parent(id INTEGER PRIMARY KEY, path VARCHAR(1024), scope INTEGER, FOREIGN KEY(scope) REFERENCES scope(id))",
    "COMMIT",
};

Datastore::Datastore(const unicode &path_to_datastore):
    db_(nullptr) {

    bool create_tables = !os::path::exists(path_to_datastore);

    int rc = sqlite3_open(path_to_datastore.encode().c_str(), &db_);
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
        SQL.encode().c_str(),
        -1,
        &stmt,
        0
    );

    assert(!ret);

    sqlite3_bind_text(stmt, 1, path.encode().c_str(), path.encode().length(), SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        throw RuntimeError("Unable to delete the selected scopes");
    }
}

void Datastore::save_scopes(const std::vector<ScopePtr>& scopes, const unicode &filename) {
    const unicode SCOPE_INSERT_SQL = "INSERT INTO scope (filename, path) VALUES(?, ?)";
    const unicode SCOPE_PARENT_SQL = "INSERT INTO scope_parent(scope, path) VALUES(?, ?)";

    for(auto scope: scopes) {
        sqlite3_stmt* stmt;
        int ret = sqlite3_prepare(db_, SCOPE_INSERT_SQL.encode().c_str(), -1, &stmt, 0);
        assert(!ret);
        sqlite3_bind_text(stmt, 1, filename.encode().c_str(), filename.encode().length(), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, scope->path().encode().c_str(), scope->path().encode().length(), SQLITE_TRANSIENT);

        if(sqlite3_step(stmt) != SQLITE_DONE) {
            throw RuntimeError("Unable to insert a scope");
        }

        int pk = sqlite3_last_insert_rowid(db_);

        for(auto path: scope->inherited_paths()) {
            ret = sqlite3_prepare(db_, SCOPE_PARENT_SQL.encode().c_str(), -1, &stmt, 0);
            assert(!ret);
            sqlite3_bind_int(stmt, 1, pk);
            sqlite3_bind_text(stmt, 2, path.encode().c_str(), path.encode().length(), SQLITE_TRANSIENT);
            if(sqlite3_step(stmt) != SQLITE_DONE) {
                throw RuntimeError("Unable to insert an inherited scope");
            }
        }
    }
}

std::vector<unicode> Datastore::query_completions(const unicode &filename, int line_number, int col_number, const unicode &string_to_complete) {
    return std::vector<unicode>();
}

}
