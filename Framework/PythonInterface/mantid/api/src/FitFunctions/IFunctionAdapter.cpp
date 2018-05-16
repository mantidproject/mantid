#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"
#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace Mantid {
namespace PythonInterface {
using API::IFunction;
using PythonInterface::Environment::UndefinedAttributeError;
using PythonInterface::Environment::callMethod;
using PythonInterface::Environment::callMethodNoCheck;
using namespace boost::python;

namespace {

/**
 * Create an Attribute from a python value.
 * @param value :: A value python object. Allowed python types:
 * float,int,str,bool.
 * @return :: The created attribute.
 */
IFunction::Attribute createAttributeFromPythonValue(const object &value) {

  PyObject *rawptr = value.ptr();
  IFunction::Attribute attr;

  if (PyBool_Check(rawptr) == 1) {
    attr = IFunction::Attribute(extract<bool>(rawptr)());
  }
#if PY_MAJOR_VERSION >= 3
  else if (PyLong_Check(rawptr) == 1) {
#else
  else if (PyInt_Check(rawptr) == 1) {
#endif
    attr = IFunction::Attribute(extract<int>(rawptr)());
  } else if (PyFloat_Check(rawptr) == 1) {
    attr = IFunction::Attribute(extract<double>(rawptr)());
  }
#if PY_MAJOR_VERSION >= 3
  else if (PyUnicode_Check(rawptr) == 1) {
#else
  else if (PyBytes_Check(rawptr) == 1) {
#endif
    attr = IFunction::Attribute(extract<std::string>(rawptr)());
  } else if (PyList_Check(rawptr) == 1) {
    auto n = PyList_Size(rawptr);
    std::vector<double> vec;
    for (Py_ssize_t i = 0; i < n; ++i) {
      auto v = extract<double>(PyList_GetItem(rawptr, i))();
      vec.push_back(v);
    }
    attr = IFunction::Attribute(vec);
  } else {
    throw std::invalid_argument("Invalid attribute type. Allowed "
                                "types=float,int,str,bool,list(float)");
  }
  return attr;
}

} // namespace

/**
 * Construct the wrapper and stores the reference to the PyObject
 * @param self A reference to the calling Python object
 * @param functionMethod The name of the function method that must be present
 * @param derivMethod The name of the derivative method that may be overridden
 */
IFunctionAdapter::IFunctionAdapter(PyObject *self, std::string functionMethod,
                                   std::string derivMethod)
    : m_self(self), m_functionName(std::move(functionMethod)),
      m_derivName(derivMethod), m_derivOveridden(Environment::typeHasAttribute(
                                    self, m_derivName.c_str())) {
  if (!Environment::typeHasAttribute(self, "init"))
    throw std::runtime_error("Function does not define an init method.");
  if (!Environment::typeHasAttribute(self, m_functionName.c_str()))
    throw std::runtime_error("Function does not define a " + m_functionName +
                             " method.");
}

/**
 * @returns The class name of the function. This cannot be overridden in Python.
 */
std::string IFunctionAdapter::name() const {
  return getSelf()->ob_type->tp_name;
}

/**
 * Specify a category for the function
 */
const std::string IFunctionAdapter::category() const {
  try {
    return callMethod<std::string>(getSelf(), "category");
  } catch (UndefinedAttributeError &) {
    return IFunction::category();
  }
}

/**
 */
void IFunctionAdapter::init() { callMethodNoCheck<void>(getSelf(), "init"); }

/**
 * Declare an attribute on the given function from a python object
 * @param name :: The name of the new attribute
 * @param defaultValue :: The default value for the attribute
 */
void IFunctionAdapter::declareAttribute(const std::string &name,
                                        const object &defaultValue) {
  auto attr = createAttributeFromPythonValue(defaultValue);
  IFunction::declareAttribute(name, attr);
  try {
    callMethod<void, std::string, object>(getSelf(), "setAttributeValue", name,
                                          defaultValue);
  } catch (UndefinedAttributeError &) {
    // nothing to do
  }
}

/**
 * Get the value of the named attribute as a Python object
 * @param self :: A reference to a function object that has the attribute.
 * @param name :: The name of the new attribute.
 * @returns The value of the attribute
 */
PyObject *IFunctionAdapter::getAttributeValue(IFunction &self,
                                              const std::string &name) {
  auto attr = self.getAttribute(name);
  return getAttributeValue(self, attr);
}

/**
 * Get the value of the given attribute as a Python object
 * @param self :: A reference to a function object that has the attribute.
 * @param attr An attribute object
 * @returns The value of the attribute
 */
PyObject *
IFunctionAdapter::getAttributeValue(IFunction &self,
                                    const API::IFunction::Attribute &attr) {
  UNUSED_ARG(self);
  std::string type = attr.type();
  PyObject *result(nullptr);
  if (type == "int")
    result = to_python_value<const int &>()(attr.asInt());
  else if (type == "double")
    result = to_python_value<const double &>()(attr.asDouble());
  else if (type == "std::string")
    result = to_python_value<const std::string &>()(attr.asString());
  else if (type == "bool")
    result = to_python_value<const bool &>()(attr.asBool());
  else if (type == "std::vector<double>")
    result = to_python_value<const std::vector<double> &>()(attr.asVector());
  else
    throw std::runtime_error("Unknown attribute type, cannot convert C++ type "
                             "to Python. Contact developement team.");
  return result;
}

/**
 * Set the attribute's value in the default IFunction's cache
 * @param self :: A reference to a function object that has the attribute.
 * @param name :: The name of the attribute
 * @param value :: The value to set
 */
void IFunctionAdapter::setAttributePythonValue(IFunction &self,
                                               const std::string &name,
                                               const object &value) {
  self.setAttribute(name, createAttributeFromPythonValue(value));
}

/**
 * Calls setAttributeValue on the Python object if it exists otherwise calls the
 * base class method
 * @param attName The name of the attribute
 * @param attr An attribute object
 */
void IFunctionAdapter::setAttribute(const std::string &attName,
                                    const Attribute &attr) {
  try {
    object value = object(handle<>(getAttributeValue(*this, attr)));
    callMethod<void, std::string, object>(getSelf(), "setAttributeValue",
                                          attName, value);
    storeAttributeValue(attName, attr);
  } catch (UndefinedAttributeError &) {
    IFunction::setAttribute(attName, attr);
  }
}

/** Split this function (if needed) into a list of independent functions.
 * @param self :: A reference to a function object. If it's a multi-domain
 *    function the result should in general contain more than 1 function.
 *    For a single domain function it should have a single element (self).
 * @return A python list of IFunction_sprs.
 */
boost::python::object
IFunctionAdapter::createPythonEquivalentFunctions(IFunction &self) {
  auto functions = self.createEquivalentFunctions();
  boost::python::list list;
  for (const auto &fun : functions) {
    list.append(fun);
  }
  return list;
}

/**
 * Value of i-th active parameter. If this functions is overridden
 * in Python then it returns the value of the ith active Parameter
 * If not it simple returns the base class result
 * @param i The index of the parameter
 */
double IFunctionAdapter::activeParameter(size_t i) const {
  try {
    return callMethod<double, size_t>(getSelf(), "activeParameter", i);
  } catch (UndefinedAttributeError &) {
    return IFunction::activeParameter(i);
  }
}

/**
 * Sets the value of i-th active parameter. If this functions is overridden
 * in Python then it should set the value of the ith active parameter.
 * If not calls the base class function
 * @param i The index of the parameter
 * @param value The new value of the active parameter
 */
void IFunctionAdapter::setActiveParameter(size_t i, double value) {
  try {
    callMethod<void, size_t, double>(getSelf(), "setActiveParameter", i, value);
  } catch (UndefinedAttributeError &) {
    IFunction::setActiveParameter(i, value);
  }
}

/**
 * The result is copied from the numpy array that is returned into the
 * provided output array
 * @param out A pre-sized array to accept the result values
 * @param xValues The input domain
 * @param nData The size of the input and output arrays
 */
void IFunctionAdapter::evaluateFunction(double *out, const double *xValues,
                                        const size_t nData) const {
  using namespace Converters;
  // GIL must be held while numpy wrappers are destroyed as they access Python
  // state information
  Environment::GlobalInterpreterLock gil;

  Py_intptr_t dims[1] = {static_cast<Py_intptr_t>(nData)};
  PyObject *xvals =
      WrapReadOnly::apply<double>::createFromArray(xValues, 1, dims);

  // Deliberately avoids using the CallMethod wrappers. They lock the GIL again
  // and
  // will check for each function call whether the wrapped method exists. It
  // also avoid unnecessary construction of
  // boost::python::objects whn using boost::python::call_method

  PyObject *result =
      PyObject_CallMethod(getSelf(), const_cast<char *>(m_functionName.c_str()),
                          const_cast<char *>("(O)"), xvals);
  Py_DECREF(xvals);
  if (PyErr_Occurred()) {
    Py_XDECREF(result);
    throw Environment::PythonException();
  }
  if (PyArray_Check(result)) {
    auto nparray = reinterpret_cast<PyArrayObject *>(result);
    // dtype matches so use memcpy for speed
    if (PyArray_TYPE(nparray) == NPY_DOUBLE) {
      std::memcpy(static_cast<void *>(out), PyArray_DATA(nparray),
                  nData * sizeof(npy_double));
      Py_DECREF(result);
    } else {
      Py_DECREF(result);
      PyArray_Descr *dtype = PyArray_DESCR(nparray);
      std::string err("Unsupported numpy data type: '");
      err.append(dtype->typeobj->tp_name)
          .append("'. Currently only numpy.float64 is supported.");
      throw std::runtime_error(err);
    }
  } else {
    std::string err("Expected ");
    err.append(m_functionName)
        .append(" to return a numpy array, however an ")
        .append(result->ob_type->tp_name)
        .append(" was returned.");
    throw std::runtime_error(err);
  }
}

/**
 * If the method does not exist on the derived type then then base
 * implementation is called
 * @param out Am output Jacobian accept the result values
 * @param xValues The input domain
 * @param nData The size of the input and output arrays
 */
void IFunctionAdapter::evaluateDerivative(API::Jacobian *out,
                                          const double *xValues,
                                          const size_t nData) const {
  using namespace Converters;
  // GIL must be held while numpy wrappers are destroyed as they access Python
  // state information
  Environment::GlobalInterpreterLock gil;

  Py_intptr_t dims[1] = {static_cast<Py_intptr_t>(nData)};
  PyObject *xvals =
      WrapReadOnly::apply<double>::createFromArray(xValues, 1, dims);
  PyObject *jacobian = boost::python::to_python_value<API::Jacobian *>()(out);

  // Deliberately avoids using the CallMethod wrappers. They lock the GIL
  // again and
  // will check for each function call whether the wrapped method exists. It
  // also avoid unnecessary construction of
  // boost::python::objects when using boost::python::call_method
  PyObject_CallMethod(getSelf(), const_cast<char *>(m_derivName.c_str()),
                      const_cast<char *>("(OO)"), xvals, jacobian);
  if (PyErr_Occurred())
    throw Environment::PythonRuntimeError();
}
} // namespace PythonInterface
} // namespace Mantid
