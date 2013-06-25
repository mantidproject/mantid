#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#include <boost/python/class.hpp>

namespace Mantid
{
  namespace PythonInterface
  {
    using Mantid::PythonInterface::Environment::CallMethod0;
    using Mantid::PythonInterface::Environment::CallMethod1;
    using Mantid::PythonInterface::Environment::CallMethod2;
    using namespace boost::python;

    /**
     * Construct the wrapper and stores the reference to the PyObject
     * @param self A reference to the calling Python object
     */
    IFunctionAdapter::IFunctionAdapter(PyObject* self)
      : IFunction(), m_name(self->ob_type->tp_name), m_self(self)
    {
    }

    /**
     * @returns The class name of the function. This cannot be overridden in Python.
     */
    std::string IFunctionAdapter::name() const
    {
      return m_name;
    }

    /**
     * Specify a category for the function
     */
    const std::string IFunctionAdapter::category() const
    {
      return CallMethod0<std::string>::dispatchWithDefaultReturn(getSelf(),"category", IFunction::category());
    }

    /**
     */
    void IFunctionAdapter::init()
    {
      CallMethod0<void>::dispatchWithException(getSelf(),"init");
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
     * Get the value of the named attribute as a Python object
     * @param name :: The name of the new attribute
     * @returns The value of the attribute
     */
    PyObject * IFunctionAdapter::getAttributeValue(const std::string & name)
    {
      auto attr = IFunction::getAttribute(name);
      return getAttributeValue(attr);
    }


    /**
     * Get the value of the given attribute as a Python object
     * @param attr An attribute object
     * @returns The value of the attribute
     */
    PyObject * IFunctionAdapter::getAttributeValue(const API::IFunction::Attribute & attr)
    {
      std::string type = attr.type();
      PyObject *result(NULL);
      if(type=="int") result = to_python_value<const int&>()(attr.asInt());
      else if(type=="double") result = to_python_value<const double&>()(attr.asDouble());
      else if(type=="std::string") result = to_python_value<const std::string&>()(attr.asString());
      else if(type=="bool") result = to_python_value<const bool&>()(attr.asBool());
      else throw std::runtime_error("Unknown attribute type, cannot convert C++ type to Python. Contact developement team.");
      return result;
    }

    /**
     * Calls setAttributeValue on the Python object if it exists otherwise calls the base class method
     * @param attName The name of the attribute
     * @param attr An attribute object
     */
    void IFunctionAdapter::setAttribute(const std::string& attName,const Attribute& attr)
    {
      object value = object(handle<>(getAttributeValue(attr)));
      try
      {
        CallMethod2<void,std::string,object>::dispatchWithException(getSelf(), "setAttributeValue", attName,value);
      }
      catch(std::runtime_error &)
      {
        IFunction::setAttribute(attName, attr);
      }
    }


    /**
     * Value of i-th active parameter. If this functions is overridden
     * in Python then it returns the value of the ith active Parameter
     * If not it simple returns the base class result
     * @param i The index of the parameter
     */
    double IFunctionAdapter::activeParameter(size_t i) const
    {
      return CallMethod1<double,size_t>::dispatchWithDefaultReturn(getSelf(), "activeParameter",
          this->getParameter(i), i);
    }

    /**
     * Sets the value of i-th active parameter. If this functions is overridden
     * in Python then it should set the value of the ith active parameter.
     * If not calls the base class function
     * @param i The index of the parameter
     * @param value The new value of the active parameter
     */
    void IFunctionAdapter::setActiveParameter(size_t i, double value)
    {
      try
      {
        CallMethod2<void,size_t,double>::dispatchWithException(getSelf(), "setActiveParameter", i,value);
      }
      catch(std::runtime_error&)
      {
        IFunction::setActiveParameter(i,value);
      }

    }


  }
}
