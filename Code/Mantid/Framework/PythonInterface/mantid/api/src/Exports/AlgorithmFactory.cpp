#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using Mantid::API::AlgorithmFactoryImpl;
using Mantid::API::AlgorithmFactory;
using namespace boost::python;

namespace
{
  ///@cond

  //------------------------------------------------------------------------------------------------------
  /**
   * A Python friendly version of get_keys that returns the registered algorithms as
   * a dictionary where the key is the algorithm name and the value is a list
   * of version numbers
   * @param self :: Enables it to be called as a member function on the AlgorithmFactory class
   * @param includeHidden :: If true hidden algorithms are included
   */
  PyObject * getRegisteredAlgorithms(AlgorithmFactoryImpl & self, bool includeHidden)
  {
    // A list of strings AlgorithmName|version
    std::vector<std::string> keys = self.getKeys(includeHidden);
    const size_t nkeys = keys.size();
    PyObject *registered = PyDict_New();
    for( size_t i = 0; i < nkeys; ++i )
    {
      std::pair<std::string, int> algInfo = self.decodeName(keys[i]);
      PyObject *name = PyString_FromString(algInfo.first.c_str());
      PyObject *vers = PyInt_FromLong(algInfo.second);
      if( PyDict_Contains(registered, name) == 1 )
      {
        // A list already exists, create a copy and add this version
        PyObject *versions = PyDict_GetItem(registered, name);
        PyList_Append(versions, vers);
      }
      else
      {
        // No entry exists, create a key and tuple
        PyObject *versions = PyList_New(1);
        PyList_SetItem(versions, 0, vers);
        PyDict_SetItem(registered, name, versions);
      }
    }

    return registered;
  }
  ///@endcond
}

void export_AlgorithmFactory()
{

  class_<AlgorithmFactoryImpl,boost::noncopyable>("AlgorithmFactory", no_init)
      .def("Instance", &AlgorithmFactory::Instance, return_value_policy<reference_existing_object>(), //This policy is really only safe for singletons
        "Returns a reference to the AlgorithmFactory singleton")
      .staticmethod("Instance")
      .def("getRegisteredAlgorithms", &getRegisteredAlgorithms, "Returns a Python dictionary of ")
    ;

}

//--------------------------------------------- Python algorithm registration ------------------------------------------------

namespace
{
  // A function to register an algorithm from Python
  void registerAlgorithm(boost::python::object obj)
  {
    using Mantid::PythonInterface::PythonObjectInstantiator;
    using Mantid::API::Algorithm;
    // The current frame should know what an Algorithm is, or it
    // couldn't create one. Get the class object from the f_globals
    PyObject *defs = PyEval_GetFrame()->f_globals;
    PyObject *pyalgClass = PyDict_GetItemString(defs, "PythonAlgorithm");
    if( !pyalgClass )
    {
      throw std::runtime_error("Unable to find Algorithm definition, cannot register algorithm.\nHas the definition been imported");
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
}


void export_RegisterAlgorithm()
{
  // The registration function
  def("registerAlgorithm", &registerAlgorithm, "Register an algorithm with Mantid. The class must derive from mantid.api.PythonAlgorithm.");
}
