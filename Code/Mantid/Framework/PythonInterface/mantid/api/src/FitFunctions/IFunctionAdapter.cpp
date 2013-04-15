#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"

#include <boost/python/class.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    using namespace boost::python;

    /**
     * Construct the wrapper and stores the reference to the PyObject
     * * @param self A reference to the calling Python object
     */
    IFunctionAdapter::IFunctionAdapter(PyObject* self)
      : IFunction(), m_name(self->ob_type->tp_name)
    {
    }

    /**
     * Returns the class name of the function. This cannot be overridden in Python.
     */
    std::string IFunctionAdapter::name() const
    {
      return m_name;
    }

    /**
     * Declare an attribute on the given function from a python object
     * @param name :: The name of the new attribute
     * @param defaultValue :: The default value for the attribute 
     */
    void IFunctionAdapter::declareAttribute(const std::string &name, 
                                            const object &defaultValue)
    {
      PyObject *rawptr = defaultValue.ptr();
      IFunction::Attribute attr;
      if(PyInt_Check(rawptr) == 1) attr = IFunction::Attribute(extract<int>(rawptr)());
      else if(PyFloat_Check(rawptr) == 1)  attr = IFunction::Attribute(extract<double>(rawptr)());
      else if(PyString_Check(rawptr) == 1) attr = IFunction::Attribute(extract<std::string>(rawptr)());
      else if(PyBool_Check(rawptr) == 1) attr = IFunction::Attribute(extract<bool>(rawptr)());
      else throw std::invalid_argument("Invalid attribute type. Allowed types=float,int,str,bool");
      
      IFunction::declareAttribute(name, attr);
    }

    /**
     * Get the value of the given attribute as a Python object
     * @param name :: The name of the new attribute
     * @param defaultValue :: The default value for the attribute
     * @returns The value of the attribute
     */
    PyObject * IFunctionAdapter::getAttributeValue(const std::string & name)
    {
      auto attr = IFunction::getAttribute(name);
      std::string type = attr.type();
      PyObject *result(NULL);
      if(type=="int") result = to_python_value<const int&>()(attr.asInt());
      else if(type=="double") result = to_python_value<const double&>()(attr.asDouble());
      else if(type=="std::string") result = to_python_value<const std::string&>()(attr.asString());
      else if(type=="bool") result = to_python_value<const bool&>()(attr.asBool());
      else throw std::runtime_error("Unknown attribute type, cannot convert C++ type to Python. Contact developement team.");
      
      return result;
    }

  }
}
