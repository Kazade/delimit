#include <vector>
#include <fstream>
#include <string>
#include <kazbase/logging.h>

#include "utils.h"

unicode call_command(unicode command, unicode cwd) {
    char tmpname [L_tmpnam];
    std::tmpnam ( tmpname );
    std::string scommand = command.encode();
    if(!cwd.empty()) {
        scommand = _u("cd {0}; {1}").format(cwd, scommand).encode();
    }
    std::string cmd = scommand + " &> " + tmpname;
    std::system(cmd.c_str());
    std::ifstream file(tmpname, std::ios::in );
    std::vector<unicode> lines;
    while(file.good()) {
        std::string line;
        std::getline(file, line);
        lines.push_back(line);
    }

    file.close();

    remove(tmpname);

    unicode ret = _u("\n").join(lines);

    return ret;
}


Glib::RefPtr<Gsv::Language> guess_language_from_file(const Glib::RefPtr<Gio::File>& file) {
    Glib::RefPtr<Gsv::LanguageManager> lm = Gsv::LanguageManager::get_default();
    Glib::RefPtr<Gsv::Language> lang = lm->guess_language(file->get_path(), Glib::ustring());

    if(lang) {
        L_DEBUG(_u("Detected {0}").format(lang->get_name()));
    } else {
        //Extra checks

        std::vector<char> buffer(1024);
        file->read()->read(&buffer[0], 1024);

        auto line = unicode(buffer.begin(), buffer.end()).split("\n")[0].lower();
        auto language_ids = lm->get_language_ids();

        if(line.starts_with("#!")) {
            if(line.contains("python")) {
                lang = lm->get_language("python");
            } else {
                L_INFO(_u("Unrecognized #! {}").format(line));
            }
        }
    }
    return lang;
}
