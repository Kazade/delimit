#include "utils.h"

#include <vector>
#include <fstream>
#include <string>

unicode call_command(unicode command, unicode cwd) {
    char tmpname [L_tmpnam];
    std::tmpnam ( tmpname );
    std::string scommand = command.encode();
    if(!cwd.empty()) {
        scommand = _u("cd {0}; {1}").format(cwd, scommand).encode();
    }
    std::string cmd = scommand + " >> " + tmpname;
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
