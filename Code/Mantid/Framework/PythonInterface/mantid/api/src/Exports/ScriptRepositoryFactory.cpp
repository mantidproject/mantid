#include "MantidAPI/ScriptRepositoryFactory.h"
#include "MantidAPI/ScriptRepository.h"
//#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
//#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

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
