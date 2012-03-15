#include "MantidAPI/AlgorithmManager.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::AlgorithmManagerImpl;
using Mantid::API::AlgorithmManager;
using namespace boost::python;

namespace
{
  ///@cond
  //------------------------------------------------------------------------------------------------------
  /// Define overload generators
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(create_overloads,AlgorithmManagerImpl::create, 1,2);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createUnmanaged_overloads,AlgorithmManagerImpl::createUnmanaged, 1,2);
  ///@endcond
}

void export_AlgorithmManager()
{
  class_<AlgorithmManagerImpl,boost::noncopyable>("AlgorithmManagerImpl", no_init)
    .def("create", &AlgorithmManagerImpl::create, create_overloads(args("name", "version"), "Creates a managed algorithm."))
    .def("createUnmanaged", &AlgorithmManagerImpl::createUnmanaged,
        createUnmanaged_overloads(args("name", "version"), "Creates an unmanaged algorithm."))
    .def("Instance", &AlgorithmManager::Instance, return_value_policy<reference_existing_object>(),
        "Returns a reference to the AlgorithmManager singleton")
    .staticmethod("Instance")
  ;
}
