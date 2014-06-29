#include <kazbase/unicode.h>
#include <kazbase/os/path.h>
#include <kazbase/exceptions.h>
#include <kazbase/hash/md5.h>
#include <cassert>
#include <gtkmm.h>
#include "datastore.h"

namespace delimit {

std::vector<unicode> INITIAL_DATA_SQL = {
    "BEGIN",
    "CREATE TABLE version(version VARCHAR(32) PRIMARY KEY)",
    "CREATE TABLE scope(id INTEGER PRIMARY KEY, filename VARCHAR(255) NOT NULL, path VARCHAR(1024) NOT NULL, start_line INTEGER NOT NULL, start_col INTEGER NOT NULL, end_line INTEGER NOT NULL, end_col INTEGER NOT NULL, parser VARCHAR(255) NOT NULL, UNIQUE(filename, path, start_line, end_line))",
    "CREATE INDEX filename_idx ON scope(path)",
    "CREATE TABLE scope_parent(id INTEGER PRIMARY KEY, path VARCHAR(1024), scope INTEGER, FOREIGN KEY(scope) REFERENCES scope(id))",
    "COMMIT",
};

unicode version() {
    return hashlib::MD5(_u("\n").join(INITIAL_DATA_SQL).encode()).hex_digest();
}

Datastore::Datastore(const unicode &path_to_datastore):
    db_(nullptr) {

    open_and_recreate_if_necessary(path_to_datastore);
}

void Datastore::open_and_recreate_if_necessary(const unicode &path_to_datastore) {
    bool create_tables = !os::path::exists(path_to_datastore);

    int rc = sqlite3_open(path_to_datastore.encode().c_str(), &db_);
    if(rc) {
        sqlite3_close(db_);
        throw IOError("Unable to create database");
    }

    if(query_database_version() != version()) {
        L_DEBUG("Deleting existing database as version differs");
        sqlite3_close(db_);
        os::remove(path_to_datastore);

        rc = sqlite3_open(path_to_datastore.encode().c_str(), &db_);
        if(rc) {
            sqlite3_close(db_);
            throw IOError("Unable to create database");
        }
        create_tables = true;
    }

    if(create_tables) {
        initialize_tables();
    }
}

unicode Datastore::query_database_version() {
    unicode sql = "SELECT version FROM version";

    sqlite3_stmt* stmt = nullptr;

    unicode vers;

    int ret = sqlite3_prepare_v2(db_, sql.encode().c_str(), -1, &stmt, 0);

    if(ret == SQLITE_OK) {
        if(sqlite3_step(stmt) == SQLITE_ROW) {
            vers = (char*) sqlite3_column_text(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return vers;
}

void Datastore::initialize_tables() {
    std::string final_statement = _u("; ").join(INITIAL_DATA_SQL).encode();
    int ret = sqlite3_exec(db_, final_statement.c_str(), 0, 0, 0);
    if(ret) {
        throw IOError("Unable to create database tables");
    }

    unicode version_sql = "INSERT INTO version (version) VALUES (?);";
    sqlite3_stmt* stmt;
    ret = sqlite3_prepare(db_, version_sql.encode().c_str(), -1, &stmt, 0);
    if(ret) {
        throw RuntimeError("Unable to prep statement to insert version");
    }

    std::string vers = version().encode();
    sqlite3_bind_text(stmt, 1, vers.c_str(), vers.length(), SQLITE_TRANSIENT);

    if(sqlite3_step(stmt) != SQLITE_DONE) {
        throw RuntimeError("Unable to insert version");
    }

    sqlite3_finalize(stmt);
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

    sqlite3_bind_text(stmt, 1, path.encode().c_str(), path.encode().length(), SQLITE_TRANSIENT);
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        throw RuntimeError(_u("Unable to delete the selected scopes: {0}").format(sqlite3_errmsg(db_)).encode());
    }
    sqlite3_finalize(stmt);
}

void Datastore::save_scopes(const unicode &parser_name, const std::vector<ScopePtr>& scopes, const unicode &filename) {
    const unicode SCOPE_INSERT_SQL = "REPLACE INTO scope (filename, path, start_line, start_col, end_line, end_col, parser) VALUES(?, ?, ?, ?, ?, ?, ?)";
    const unicode SCOPE_PARENT_SQL = "INSERT INTO scope_parent(scope, path) VALUES(?, ?)";

    sqlite3_exec(db_, "BEGIN;", 0, 0, 0);
    for(auto scope: scopes) {
        sqlite3_stmt* stmt = nullptr;
        int ret = sqlite3_prepare(db_, SCOPE_INSERT_SQL.encode().c_str(), -1, &stmt, 0);
        assert(!ret);
        sqlite3_bind_text(stmt, 1, filename.encode().c_str(), filename.encode().length(), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, scope->path().encode().c_str(), scope->path().encode().length(), SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, scope->start_line);
        sqlite3_bind_int(stmt, 4, scope->start_col);
        sqlite3_bind_int(stmt, 5, scope->end_line);
        sqlite3_bind_int(stmt, 6, scope->end_col);
        sqlite3_bind_text(stmt, 7, parser_name.encode().c_str(), parser_name.encode().length(), SQLITE_TRANSIENT);

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

        //Keep the window responsive
        while(Gtk::Main::events_pending()) {
            Gtk::Main::iteration();
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_exec(db_, "COMMIT;", 0, 0, 0);
}

unicode Datastore::query_scope_at(const unicode& parser, const unicode &filename, int line_number, int col_number) {
    return "";
}

std::vector<unicode> Datastore::query_completions(const unicode& parser, const unicode &filename, int line_number, int col_number, const unicode &string_to_complete) {
    //FIXME: Must take into account scope! This only works for the PLAIN parser search not more complex lookups
    std::cout << parser << ": " << string_to_complete << std::endl;

    unicode sql = "SELECT DISTINCT path FROM scope WHERE parser = ? AND path LIKE ? ORDER BY path;";

    sqlite3_stmt* stmt = nullptr;

    int ret = sqlite3_prepare_v2(db_, sql.encode().c_str(), -1, &stmt, 0);
    if(ret != SQLITE_OK) {
        return std::vector<unicode>();
    }

    std::string like = _u("{0}%").format(string_to_complete).encode();
    std::string par = parser.encode();

    sqlite3_bind_text(stmt, 1, par.c_str(), par.length(), SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, like.c_str(), like.length(), SQLITE_TRANSIENT);

    std::vector<unicode> results;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        unicode path = (char*) sqlite3_column_text(stmt, 0);
        results.push_back(path);
    }

    sqlite3_finalize(stmt);
    return results;
}

}
