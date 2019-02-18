#ifndef UTILS_H
#define UTILS_H

#include <gtkmm.h>
#include <gtksourceviewmm.h>
#include "utils/unicode.h"

unicode call_command(unicode command, unicode cwd="");
Glib::RefPtr<Gsv::Language> guess_language_from_file(const Glib::RefPtr<Gio::File>& file);

#endif // UTILS_H
