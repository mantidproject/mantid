#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ScriptRepository.h"

#include <boost/python/class.hpp>

using Mantid::API::ScriptRepositoryFactoryImpl;
using Mantid::API::ScriptRepositoryFactory;
using namespace boost::python;

namespace
{
  ///@cond

  //------------------------------------------------------------------------------------------------------

  ///@endcond
}

void export_ScriptRepositoryFactory()
{
  class_<ScriptRepositoryFactoryImpl,boost::noncopyable>("ScriptRepositoryFactory", no_init)
      .def("create", &ScriptRepositoryFactoryImpl::create,
           "Return a pointer to the ScriptRepository object")
      .def("Instance", &ScriptRepositoryFactory::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the ScriptRepositoryFactory singleton")
      .staticmethod("Instance")
    ;

}
