#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <Poco/ScopedLock.h>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using namespace Mantid::API;
using namespace boost::python;
using Mantid::Kernel::AbstractInstantiator;
using Mantid::PythonInterface::PythonObjectInstantiator;

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

  //--------------------------------------------- Python algorithm subscription ------------------------------------------------

  // Python algorithm registration mutex in anonymous namespace (aka static)
  Poco::Mutex PYALG_REGISTER_MUTEX;

GCC_DIAG_OFF(cast-qual)
  /**
   * A free function to subscribe a Python algorithm into the factory
   * @param obj :: A Python object that should either be a class type derived from PythonAlgorithm
   *              or an instance of a class type derived from PythonAlgorithm
   */
  void subscribe(AlgorithmFactoryImpl & self, const boost::python::object & obj)
  {
    Poco::ScopedLock<Poco::Mutex> lock(PYALG_REGISTER_MUTEX);

    static PyObject * const pyAlgClass =
        (PyObject*)converter::registered<Algorithm>::converters.to_python_target_type();
    // obj could be or instance/class, check instance first
    PyObject *classObject(NULL);
    if( PyObject_IsInstance(obj.ptr(), pyAlgClass) )
    {
      classObject = PyObject_GetAttrString(obj.ptr(), "__class__");
    }
    else if(PyObject_IsSubclass(obj.ptr(), pyAlgClass))
    {
      classObject = obj.ptr(); // We need to ensure the type of lifetime management so grab the raw pointer
    }
    else
    {
      throw std::invalid_argument("Cannot register an algorithm that does not derive from Algorithm.");
    }
    boost::python::object classType(handle<>(borrowed(classObject)));
    // Takes ownership of instantiator and replaces any existing algorithm
    auto descr = self.subscribe(new PythonObjectInstantiator<Algorithm>(classType), AlgorithmFactoryImpl::OverwriteCurrent);

    // Python algorithms cannot yet act as loaders so remove any registered ones from the FileLoaderRegistry
    FileLoaderRegistry::Instance().unsubscribe(descr.first, descr.second);
  }

  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(existsOverloader, exists, 1, 2)

  ///@endcond
}
GCC_DIAG_ON(cast-qual)

void export_AlgorithmFactory()
{

  class_<AlgorithmFactoryImpl,boost::noncopyable>("AlgorithmFactoryImpl", no_init)
      .def("exists", &AlgorithmFactoryImpl::exists,
           existsOverloader((arg("name"), arg("version")=-1),
                            "Returns true if the given algorithm exists with an option to specify the version"))

      .def("getRegisteredAlgorithms", &getRegisteredAlgorithms, "Returns a Python dictionary of currently registered algorithms")
      .def("highestVersion", &AlgorithmFactoryImpl::highestVersion,
           "Returns the highest version of the named algorithm. Throws ValueError if no algorithm can be found")
      .def("subscribe", &subscribe, "Register a Python class derived from PythonAlgorithm into the factory")

      .def("Instance", &AlgorithmFactory::Instance, return_value_policy<reference_existing_object>(),
        "Returns a reference to the AlgorithmFactory singleton")
      .staticmethod("Instance")
    ;

}
