#ifndef SIGC_LAMBDA_H
#define SIGC_LAMBDA_H

#include <gtk/gtk.h>
#include <type_traits>
#include <sigc++/sigc++.h>

#if GTK_CHECK_VERSION(3,18,0)
    // Nothing to do here!
#else
namespace sigc
{
   template <typename Functor>
       struct functor_trait<Functor, false>
       {
           typedef decltype (::sigc::mem_fun (std::declval<Functor&> (),
                       &Functor::operator())) _intermediate;

           typedef typename _intermediate::result_type result_type;
           typedef Functor functor_type;
       };
}
#endif
#endif // SIGC_LAMBDA_H
