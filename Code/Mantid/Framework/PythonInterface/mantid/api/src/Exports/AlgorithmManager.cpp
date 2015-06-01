#include "MantidAPI/AlgorithmManager.h"
#include "MantidPythonInterface/api/AlgorithmIDProxy.h"
#include "MantidPythonInterface/kernel/TrackingInstanceMethod.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::AlgorithmIDProxy;
using Mantid::PythonInterface::TrackingInstanceMethod;
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

  /**
   * Remove the algorithm identified by the given ID from the list of managed algorithms.
   * A wrapper version that takes a AlgorithmIDProxy that wraps a AlgorithmID
   * @param self The calling object
   * @param id An algorithm ID
   */
  void removeById(AlgorithmManagerImpl &self, AlgorithmIDProxy idHolder)
  {
    return self.removeById(idHolder.id);
  }

  /**
   * @param self A reference to the calling object
   * @param algName The name of the algorithm
   * @return A python list of managed algorithms that are currently running
   */
  boost::python::list runningInstancesOf(AlgorithmManagerImpl &self, const std::string & algName)
  {
    boost::python::list algs;
    auto mgrAlgs = self.runningInstancesOf(algName);
    for(auto iter = mgrAlgs.begin(); iter != mgrAlgs.end(); ++iter)
    {
      // boost 1.41 (RHEL6) can't handle registering IAlgorithm_const_sptr so we
      // have to cast to IAlgorithm_sptr and then convert to Python
      // The constness is pretty-irrelevant by this point anyway
      algs.append(boost::const_pointer_cast<IAlgorithm>(*iter));
    }

    return algs;
  }

  ///@cond
  //------------------------------------------------------------------------------------------------------
  /// Define overload generators
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(create_overloads,AlgorithmManagerImpl::create, 1,2)
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createUnmanaged_overloads,AlgorithmManagerImpl::createUnmanaged, 1,2)
  ///@endcond
}

void export_AlgorithmManager()
{
  typedef class_<AlgorithmManagerImpl,boost::noncopyable> PythonType;

  auto pythonClass = class_<AlgorithmManagerImpl,boost::noncopyable>("AlgorithmManagerImpl", no_init)
    .def("create", &AlgorithmManagerImpl::create, create_overloads((arg("name"), arg("version")), "Creates a managed algorithm."))
    .def("createUnmanaged", &AlgorithmManagerImpl::createUnmanaged,
        createUnmanaged_overloads((arg("name"), arg("version")), "Creates an unmanaged algorithm."))
    .def("size", &AlgorithmManagerImpl::size, "Returns the number of managed algorithms")
    .def("setMaxAlgorithms", &AlgorithmManagerImpl::setMaxAlgorithms,
         "Set the maximum number of allowed managed algorithms")
    .def("getAlgorithm", &getAlgorithm,
         "Return the algorithm instance identified by the given id.")
    .def("removeById", &removeById, "Remove an algorithm from the managed list")
    .def("newestInstanceOf", &AlgorithmManagerImpl::newestInstanceOf,
         "Returns the newest created instance of the named algorithm")
    .def("runningInstancesOf", &runningInstancesOf,
         "Returns a list of managed algorithm instances that are currently executing")
    .def("clear", &AlgorithmManagerImpl::clear, "Clears the current list of managed algorithms")
    .def("cancelAll", &AlgorithmManagerImpl::cancelAll,
         "Requests that all currently running algorithms be cancelled")
  ;

  // Instance method
  TrackingInstanceMethod<AlgorithmManager, PythonType>::define(pythonClass);

}
