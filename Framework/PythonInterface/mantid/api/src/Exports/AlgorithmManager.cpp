// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/AlgorithmIDProxy.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::AlgorithmIDProxy;
using Mantid::PythonInterface::ReleaseGlobalInterpreterLock;
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
                       "def cleanupAlgorithmManager():\n"
                       "    from mantid.api import AlgorithmManager\n"
                       "    AlgorithmManager.shutdown()\n"
                       "atexit.register(cleanupAlgorithmManager)");
  });
  return mgr;
}

IAlgorithm_sptr create(AlgorithmManagerImpl *self, const std::string &algName, const int &version = -1) {
  ReleaseGlobalInterpreterLock releaseGIL;
  return self->create(algName, version);
}

std::shared_ptr<Algorithm> createUnmanaged(AlgorithmManagerImpl *self, const std::string &algName,
                                           const int &version = -1) {
  ReleaseGlobalInterpreterLock releaseGIL;
  return self->createUnmanaged(algName, version);
}

void clear(AlgorithmManagerImpl *self) {
  /// TODO We should release the GIL here otherwise we risk deadlock (see issue #33895). However, doing so causes
  /// test failures because it exposes an unreleated bug to do with the way we handle shared_ptrs to Python objects
  /// (see #33924). Fixing that is not trivial, so I am reverting this change until it can be resolved properly.
  // ReleaseGlobalInterpreterLock releaseGIL;
  return self->clear();
}

void shutdown(AlgorithmManagerImpl *self) {
  // See comment above for clear()
  // ReleaseGlobalInterpreterLock releaseGIL;
  return self->shutdown();
}

void cancelAll(AlgorithmManagerImpl *self) {
  ReleaseGlobalInterpreterLock releaseGIL;
  return self->cancelAll();
}

/**
 * Return the algorithm identified by the given ID. A wrapper version that takes
 * a
 * AlgorithmIDProxy that wraps a AlgorithmID
 * @param self The calling object
 * @param id An algorithm ID
 */
IAlgorithm_sptr getAlgorithm(AlgorithmManagerImpl const *const self, AlgorithmIDProxy idHolder) {
  ReleaseGlobalInterpreterLock releaseGIL;
  return self->getAlgorithm(idHolder.id);
}

/**
 * Remove the algorithm identified by the given ID from the list of managed
 * algorithms.
 * A wrapper version that takes a AlgorithmIDProxy that wraps a AlgorithmID
 * @param self The calling object
 * @param id An algorithm ID
 */
void removeById(AlgorithmManagerImpl &self, AlgorithmIDProxy idHolder) {
  ReleaseGlobalInterpreterLock releaseGIL;
  return self.removeById(idHolder.id);
}

/**
 * @param self A reference to the calling object
 * @param algName The name of the algorithm
 * @return A python list of managed algorithms that are currently running
 */
boost::python::list runningInstancesOf(AlgorithmManagerImpl const *const self, const std::string &algName) {
  std::vector<IAlgorithm_const_sptr> mgrAlgs;

  {
    ReleaseGlobalInterpreterLock releaseGIL;
    mgrAlgs = self->runningInstancesOf(algName);
  }

  boost::python::list algs;
  for (auto &mgrAlg : mgrAlgs) {
    algs.append(mgrAlg);
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
// cppcheck-suppress-begin unknownMacro
BOOST_PYTHON_FUNCTION_OVERLOADS(create_overloads, create, 2, 3)
BOOST_PYTHON_FUNCTION_OVERLOADS(createUnmanaged_overloads, createUnmanaged, 2, 3)
// cppcheck-suppress-end unknownMacro
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
///@endcond
} // namespace

void export_AlgorithmManager() {

  class_<AlgorithmManagerImpl, boost::noncopyable>("AlgorithmManagerImpl", no_init)
      .def("create", &create, create_overloads((arg("name"), arg("version")), "Creates a managed algorithm."))
      .def("createUnmanaged", &createUnmanaged,
           createUnmanaged_overloads((arg("name"), arg("version")), "Creates an unmanaged algorithm."))
      .def("size", &AlgorithmManagerImpl::size, arg("self"), "Returns the number of managed algorithms")
      .def("getAlgorithm", &getAlgorithm, (arg("self"), arg("id_holder")),
           "Return the algorithm instance identified by the given id.")
      .def("removeById", &removeById, (arg("self"), arg("id_holder")), "Remove an algorithm from the managed list")
      .def("runningInstancesOf", &runningInstancesOf, (arg("self"), arg("algorithm_name")),
           "Returns a list of managed algorithm instances that are "
           "currently executing")
      .def("clear", &clear, arg("self"), "Clears the current list of managed algorithms")
      .def("shutdown", &shutdown, arg("self"), "Cancels all running algorithms and waits for them to exit")
      .def("cancelAll", &cancelAll, arg("self"), "Requests that all currently running algorithms be cancelled")
      .def("Instance", instance, return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
