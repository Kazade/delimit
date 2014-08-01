#ifndef AUTOCOMPLETE_BASE_H
#define AUTOCOMPLETE_BASE_H

#include <vector>
#include <memory>
#include <kazbase/unicode.h>

namespace delimit {

class Datastore;
class Scope;
class FileParser;
class Indexer;

typedef std::shared_ptr<Scope> ScopePtr;
typedef std::shared_ptr<FileParser> FileParserPtr;
typedef std::shared_ptr<Datastore> DatastorePtr;
typedef std::shared_ptr<Indexer> IndexerPtr;

class FileParser {
public:
    virtual const unicode name() const = 0;
    virtual std::pair<std::vector<ScopePtr>, bool> parse(const unicode& data, const unicode& base_scope) = 0;
    virtual unicode base_scope_from_filename(const unicode& filename) = 0;
    virtual bool supports_nested_lookups() const = 0;
};

class Scope {
public:
    Scope(const unicode& path, std::vector<unicode> inherited_scopes=std::vector<unicode>());

    const unicode path() const { return path_; }
    const std::vector<unicode> inherited_paths() const { return inherited_paths_; }

    int start_line;
    int end_line;
    int start_col;
    int end_col;

private:
    unicode path_;
    std::vector<unicode> inherited_paths_;
};


/**
   USAGE:

   auto indexer = std::make_shared<Indexer>("/tmp/name.index"); //sqlite3 database

   indexer->register_parser("py", std::make_shared<PythonParser>());
   for(auto scope: indexer->index_file("some/python/file.py")) {
        print(scope.path());
        print(scope.inherits());
   }

   ----------------
   //Built in types
   dict
   dict::keys


   file
   file::function
   file::function::self
   file::function::var1
   file::myclass
   file::myclass::method1
   file::myclass::method1::self
   file::myclass::method1::arg1

   file::myclass::method1::var1
   inherits [ dict ]

*/


}

#endif // BASE_H
