#ifndef SIGC_LAMBDA_H
#define SIGC_LAMBDA_H

#include <type_traits>
#include <sigc++/sigc++.h>
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

#endif // SIGC_LAMBDA_H
