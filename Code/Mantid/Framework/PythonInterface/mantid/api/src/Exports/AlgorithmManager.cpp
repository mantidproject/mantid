#include "MantidAPI/AlgorithmManager.h"
#include "MantidPythonInterface/api/AlgorithmIDProxy.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::AlgorithmIDProxy;
using namespace boost::python;

namespace
{
  /**
   * Return the algorithm identified by the given ID. A wrapper version that takes a
   * AlgorithmIDProxy that wraps a AlgorithmID
   * @param self The calling object
   * @param id An algorithm ID
   */
  IAlgorithm_sptr getAlgorithm(AlgorithmManagerImpl &self, AlgorithmIDProxy idHolder)
  {
    return self.getAlgorithm(idHolder.id);
  }

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
    .def("create", &AlgorithmManagerImpl::create, create_overloads((arg("name"), arg("version")), "Creates a managed algorithm."))
    .def("createUnmanaged", &AlgorithmManagerImpl::createUnmanaged,
        createUnmanaged_overloads((arg("name"), arg("version")), "Creates an unmanaged algorithm."))
    .def("size", &AlgorithmManagerImpl::size, "Returns the number of managed algorithms")
    .def("setMaxAlgorithms", &AlgorithmManagerImpl::setMaxAlgorithms,
         "Set the maximum number of allowed managed algorithms")
    .def("getAlgorithm", &getAlgorithm,
         "Return the algorithm instance identified by the given id.")

    .def("clear", &AlgorithmManagerImpl::clear, "Clears the current list of managed algorithms")
    .def("cancelAll", &AlgorithmManagerImpl::cancelAll,
         "Requests that all currently running algorithms be cancelled")

    .def("Instance", &AlgorithmManager::Instance, return_value_policy<reference_existing_object>(),
        "Returns a reference to the AlgorithmManager singleton")
    .staticmethod("Instance")
  ;
}
