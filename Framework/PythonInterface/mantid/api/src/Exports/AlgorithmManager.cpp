// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/AlgorithmIDProxy.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::AlgorithmIDProxy;
using namespace boost::python;

namespace {

std::once_flag INIT_FLAG;

/**
 * Returns a reference to the AlgorithmManager object, creating it
 * if necessary. In addition to creating the object the first call also:
 *   - register AlgorithmManager.shutdown as an atexit function
 * @return A reference to the AlgorithmManager instance
 */
AlgorithmManagerImpl &instance() {
  // start the framework (if necessary)
  auto &mgr = AlgorithmManager::Instance();
  std::call_once(INIT_FLAG, []() {
    PyRun_SimpleString("import atexit\n"
                       "from mantid.api import AlgorithmManager\n"
                       "atexit.register(lambda: AlgorithmManager.clear())");
  });
  return mgr;
}

/**
 * Return the algorithm identified by the given ID. A wrapper version that takes
 * a
 * AlgorithmIDProxy that wraps a AlgorithmID
 * @param self The calling object
 * @param id An algorithm ID
 */
IAlgorithm_sptr getAlgorithm(AlgorithmManagerImpl &self,
                             AlgorithmIDProxy idHolder) {
  return self.getAlgorithm(idHolder.id);
}

/**
 * Remove the algorithm identified by the given ID from the list of managed
 * algorithms.
 * A wrapper version that takes a AlgorithmIDProxy that wraps a AlgorithmID
 * @param self The calling object
 * @param id An algorithm ID
 */
void removeById(AlgorithmManagerImpl &self, AlgorithmIDProxy idHolder) {
  return self.removeById(idHolder.id);
}

/**
 * @param self A reference to the calling object
 * @param algName The name of the algorithm
 * @return A python list of managed algorithms that are currently running
 */
boost::python::list runningInstancesOf(AlgorithmManagerImpl &self,
                                       const std::string &algName) {
  boost::python::list algs;
  auto mgrAlgs = self.runningInstancesOf(algName);
  for (auto &mgrAlg : mgrAlgs) {
    // boost 1.41 (RHEL6) can't handle registering IAlgorithm_const_sptr so we
    // have to cast to IAlgorithm_sptr and then convert to Python
    // The constness is pretty-irrelevant by this point anyway
    algs.append(boost::const_pointer_cast<IAlgorithm>(mgrAlg));
  }

  return algs;
}

///@cond
//------------------------------------------------------------------------------------------------------
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
/// Define overload generators
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(create_overloads,
                                       AlgorithmManagerImpl::create, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createUnmanaged_overloads,
                                       AlgorithmManagerImpl::createUnmanaged, 1,
                                       2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
///@endcond
} // namespace

void export_AlgorithmManager() {

  class_<AlgorithmManagerImpl, boost::noncopyable>("AlgorithmManagerImpl",
                                                   no_init)
      .def("create", &AlgorithmManagerImpl::create,
           create_overloads((arg("name"), arg("version")),
                            "Creates a managed algorithm."))
      .def("createUnmanaged", &AlgorithmManagerImpl::createUnmanaged,
           createUnmanaged_overloads((arg("name"), arg("version")),
                                     "Creates an unmanaged algorithm."))
      .def("size", &AlgorithmManagerImpl::size, arg("self"),
           "Returns the number of managed algorithms")
      .def("setMaxAlgorithms", &AlgorithmManagerImpl::setMaxAlgorithms,
           (arg("self"), arg("n")),
           "Set the maximum number of allowed managed algorithms")
      .def("getAlgorithm", &getAlgorithm, (arg("self"), arg("id_holder")),
           "Return the algorithm instance identified by the given id.")
      .def("removeById", &removeById, (arg("self"), arg("id_holder")),
           "Remove an algorithm from the managed list")
      .def("newestInstanceOf", &AlgorithmManagerImpl::newestInstanceOf,
           (arg("self"), arg("algorithm_name")),
           "Returns the newest created instance of the named algorithm")
      .def("runningInstancesOf", &runningInstancesOf,
           (arg("self"), arg("algorithm_name")),
           "Returns a list of managed algorithm instances that are "
           "currently executing")
      .def("clear", &AlgorithmManagerImpl::clear, arg("self"),
           "Clears the current list of managed algorithms")
      .def("cancelAll", &AlgorithmManagerImpl::cancelAll, arg("self"),
           "Requests that all currently running algorithms be cancelled")
      .def("Instance", instance,
           return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
