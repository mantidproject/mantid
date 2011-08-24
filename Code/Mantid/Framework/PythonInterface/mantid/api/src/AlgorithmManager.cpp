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

  /// Define a overload generators
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(create_overloads,AlgorithmManagerImpl::create, 1,2);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createUnmanaged_overloads,AlgorithmManagerImpl::createUnmanaged, 1,2);

  // A factory function returning a reference to the AlgorithmManager instance so that
  // Python can use it
  AlgorithmManagerImpl & getAlgorithmManager()
  {
    return AlgorithmManager::Instance();
  }
  ///@endcond
}

void export_AlgorithmManager()
{
  class_<AlgorithmManagerImpl,boost::noncopyable>("AlgorithmManager", no_init)
    .def("create", &AlgorithmManagerImpl::create, create_overloads(args("name", "version"), "Creates a managed algorithm."))
    .def("create_unmanaged", &AlgorithmManagerImpl::createUnmanaged,
        createUnmanaged_overloads(args("name", "version"), "Creates an unmanaged algorithm."))
    ;

  // Create a factory function to return this in Python
  def("get_algorithm_mgr", &getAlgorithmManager, return_value_policy<reference_existing_object>(),
        "Returns a reference to the AlgorithmManager singleton")
  ;
}
