#include "indexer.h"
#include "datastore.h"

#include "../utils/kfs.h"
#include "../utils/files.h"

namespace delimit {

static const std::map<unicode, unicode> MIMETYPES = {
    { ".py", "application/python" },
};

Indexer::Indexer(const unicode& path_to_datastore) {
    datastore_ = std::make_shared<Datastore>(path_to_datastore);
}

void Indexer::register_parser(const unicode& mimetype, FileParserPtr parser) {
    parsers_.insert(std::make_pair(mimetype, parser));
    parsers_by_name_.insert(std::make_pair(parser->name(), parser));
}

unicode Indexer::guess_type(const unicode& filename) {
    /*
     *  Tries to guess the mimetype of a file
     *
     *  FIXME: Switch to Gio::content_type_guess but try to avoid the cost of
     *  loading file data twice.
     */

    auto ext = kfs::path::split_ext(filename.encode()).second;
    auto it = MIMETYPES.find(ext);
    if(it == MIMETYPES.end()) {
        return "text/plain";
    }

    return (*it).second;
}

FileParserPtr Indexer::detect_parser(const unicode& filename) {
    /*
     *  Given a filename, this method returns a matching parser,
     *  if one could not be found, it defaults to the text/plain
     *  parser
     */

    unicode mime = guess_type(filename);
    auto parser = parsers_.find(mime);

    if(parser == parsers_.end()) {
        return parsers_.at("text/plain");
    } else {
        return (*parser).second;
    }
}

std::vector<ScopePtr> Indexer::index_file(const unicode& filename, const unicode& data) {
    auto parser = detect_parser(filename);

    auto base_scope = parser->base_scope_from_filename(filename);
    auto scopes_and_success = parser->parse(data, base_scope);

    if(scopes_and_success.second) {
        datastore_->delete_scopes_by_filename(filename);
        datastore_->save_scopes(parser->name(), scopes_and_success.first, filename);
    }
    return scopes_and_success.first;
}

std::vector<ScopePtr> Indexer::index_file(const unicode &path) {
    /*
     *  Indexes a file by running a parser over it and storing the
     *  resulting scopes in the database
     */

    return index_file(path, read_file_contents(path));
}

void Indexer::index_directory(const unicode &dir_path) {
    /*
     *  Recursively indexes the files in a directory
     */

    for(auto file: kfs::path::list_dir(dir_path.encode())) {
        auto full_path = kfs::path::join(dir_path.encode(), file);

        if(kfs::path::is_dir(full_path)) {
            index_directory(full_path);
        } else {
            index_file(full_path);
        }
    }
}


}
