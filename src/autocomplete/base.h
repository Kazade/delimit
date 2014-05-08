#ifndef AUTOCOMPLETE_BASE_H
#define AUTOCOMPLETE_BASE_H

#include <vector>

#include <kazbase/unicode.h>

namespace delimit {

class Datastore;
class Scope;
class FileParser;

typedef std::shared_ptr<Scope> ScopePtr;
typedef std::shared_ptr<FileParser> FileParserPtr;
typedef std::shared_ptr<Datastore> DatastorePtr;

class FileParser {
public:
    virtual std::vector<ScopePtr> parse(const unicode& data) = 0;
};

class Scope {
public:
    Scope(const unicode& path, std::vector<unicode> inherited_scopes=std::vector<unicode>());
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
