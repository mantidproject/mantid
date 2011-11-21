#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/kernel/PyEnvironment.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>

using Mantid::API::FrameworkManagerImpl;
using Mantid::API::FrameworkManager;
using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using namespace boost::python;

namespace
{
  ///@cond

  // A factory function returning a reference to the AlgorithmManager instance so that
  // Python can use it
  FrameworkManagerImpl & getFrameworkManager()
  {
    return FrameworkManager::Instance();
  }

  /**
   * Creates an initialised algorithm.
   * If this called from within a Python, i.e if PyExec is in the call stack,
   * an unmanaged algorithm is created otherwise it will be a managed algorithm.
   * @param self :: A reference to the FrameworkManager object
   * @param name :: The name of the algorithm to create
   * @param version :: The version of the algorithm to create (default = -1 = highest version)
   */
  IAlgorithm_sptr createAlgorithm(boost::python::object self, const std::string & name, const int version = -1)
  {
    UNUSED_ARG(self);
    IAlgorithm_sptr alg;
    if( Mantid::PythonInterface::PyEnvironment::isInCallStack("PyExec") )
    {
      alg = AlgorithmManager::Instance().createUnmanaged(name, version);
      alg->initialize();
    }
    else
    {
      alg = AlgorithmManager::Instance().create(name, version); // This will be initialized already
    }
    alg->setRethrows(true);
    return alg;
  }

  //------------------------------------------------------------------------------------------------------
    /// Define overload generators
    BOOST_PYTHON_FUNCTION_OVERLOADS(create_overloads, createAlgorithm, 2,3);
  ///@endcond
}


void export_FrameworkManager()
{
  class_<FrameworkManagerImpl,boost::noncopyable>("FrameworkManager", no_init)
    .def("clear", &FrameworkManagerImpl::clear, "Clear all memory held by Mantid")
    .def("clear_algorithms", &FrameworkManagerImpl::clearAlgorithms, "Clear memory held by algorithms (does not include workspaces)")
    .def("clear_data", &FrameworkManagerImpl::clearData, "Clear memory held by the data service (essentially all workspaces, including hidden)")
    .def("clear_instruments", &FrameworkManagerImpl::clearInstruments, "Clear memory held by the cached instruments")
    // NOTE: This differs from the C++ FrameworkManager::createAlgorithm to ensure consistency when called within Python
    .def("create_algorithm", &createAlgorithm, create_overloads(args("name", "version"), "Creates and initializes an algorithm of the "
         "given name and version. If this called from within a Python algorithm an unmanaged algorithm is created otherwise it will "
         "be a managed algorithm"))
    ;

  // Create a factory function to return this in Python
  def("get_framework_mgr", &getFrameworkManager, return_value_policy<reference_existing_object>(),
        "Returns a reference to the FrameworkManager singleton")
  ;


}

