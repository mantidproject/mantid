#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/args.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

#include <frameobject.h> // Python frameobject

using Mantid::API::AlgorithmManagerImpl;
using Mantid::API::AlgorithmManager;
using Mantid::API::AlgorithmFactory;
using Mantid::API::Algorithm;
using Mantid::PythonInterface::PythonObjectInstantiator;
using namespace boost::python;

namespace
{
  ///@cond
  //------------------------------------------------------------------------------------------------------
  // A factory function returning a reference to the AlgorithmManager instance so that Python can use it
  AlgorithmManagerImpl & getAlgorithmManager()
  {
    return AlgorithmManager::Instance();
  }

  //------------------------------------------------------------------------------------------------------
  // A function to register an algorithm from Python
  void registerAlgorithm(boost::python::object obj)
  {
    // The current frame should know what a PythonAlgorithm is, or it
    // couldn't create one. Get the class object from the f_globals
    PyObject *defs = PyEval_GetFrame()->f_globals;
    PyObject *pyalgClass = PyDict_GetItemString(defs, "PythonAlgorithm");
    if( !pyalgClass )
    {
      throw std::runtime_error("Unable to find PythonAlgorithm definition, cannot register algorithm.\nHas the definition been imported into the current scope");
    }
    // obj could be or instance/class, check instance first
    PyObject *classObject(NULL);
    if( PyObject_IsInstance(obj.ptr(), pyalgClass) )
    {
      classObject = PyObject_GetAttrString(obj.ptr(), "__class__");
    }
    else if(PyObject_IsSubclass(obj.ptr(), pyalgClass))
    {
      classObject = obj.ptr(); // We need to ensure the type of lifetime management so grab the raw pointer
    }
    else
    {
      throw std::invalid_argument("Cannot register an algorithm that does not derive from PythonAlgorithm.");
    }
    boost::python::object classType(handle<>(borrowed(classObject)));
    AlgorithmFactory::Instance().subscribe(new PythonObjectInstantiator<Algorithm>(classType));
  }

  //------------------------------------------------------------------------------------------------------
  /// Define overload generators
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(create_overloads,AlgorithmManagerImpl::create, 1,2);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createUnmanaged_overloads,AlgorithmManagerImpl::createUnmanaged, 1,2);

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
  def("get_algorithm_mgr", &getAlgorithmManager, return_value_policy<reference_existing_object>(), //This policy is really only safe for singletons
        "Returns a reference to the AlgorithmManager singleton");

  // The registration function
  def("register_algorithm", &registerAlgorithm, "Register an algorithm with the Mantid algorithm factory");
}
