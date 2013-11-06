#include <gtkmm.h>
#include <gtksourceviewmm.h>


#include "base/os.h"
#include "base/logging.h"
#include "utils/sigc_lambda.h"

#include "application.h"

#include "base/glob.h"

int main(int argc, char* argv[]) {
    Glib::RefPtr<Gtk::Application> app =
        delimit::Application::create(argc, argv, "uk.co.kazade.Delimit",
            Gio::APPLICATION_HANDLES_OPEN
        );

    assert(app);

    return app->run();
}
