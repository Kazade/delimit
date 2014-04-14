#include <gtkmm.h>
#include <gtksourceviewmm.h>


#include <kazbase/os.h>
#include <kazbase/logging.h>

#include "utils/sigc_lambda.h"

#include "application.h"

#include <kazbase/glob.h>

int main(int argc, char* argv[]) {
    Glib::RefPtr<Gtk::Application> app =
        delimit::Application::create(argc, argv, "uk.co.kazade.Delimit",
            Gio::APPLICATION_HANDLES_OPEN
        );

    assert(app);

    return app->run();
}
