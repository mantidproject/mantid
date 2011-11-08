//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/VectorDelegate.h"
#include <boost/python/extract.hpp>

namespace Mantid
{
  namespace PythonInterface
  {

    /**
     * Convert a python object into a std::vector
     * @param value :: A pointer to a python list
     * @return A new vector created from the python values (i.e. a copy)
     */
    template<typename ElementType>
    const std::vector<ElementType> VectorDelegate<ElementType>::toStdVector(PyObject *value)
    {
      const std::string msg = isConvertibleToStdVector(value);
      if( !msg.empty() )
      {
        throw std::invalid_argument(msg);
      }
      Py_ssize_t len = PySequence_Size(value);
      std::vector<ElementType> result((size_t)len);
      for( Py_ssize_t i = 0; i < len; ++i )
      {
        PyObject *item = PySequence_Fast_GET_ITEM(value, i);
        ElementType element = boost::python::extract<ElementType>(item)();
        result[i]= element;
      }
      return result;
    }

    /**
     *  Check that the list contains items of the required type for the C++ type
     *  @param value :: A pointer to a python list object
     *  @return ""if the list can be converted to a vector, otherwise it returns an error string
     */
    template<typename ElementType>
    std::string VectorDelegate<ElementType>::isConvertibleToStdVector(PyObject* value)
    {
      std::string error("");
      if( !PySequence_Check(value) )
      {
        error = std::string("Cannot convert ") + value->ob_type->tp_name + " object to a std::vector.";
      }
      return error;
    }

    //--------------------------------------------------------------------------
    // Concrete Instantiations
    //--------------------------------------------------------------------------
    template DLLExport struct VectorDelegate<int>;
    template DLLExport struct VectorDelegate<double>;
  }
}

