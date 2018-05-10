#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"

#include "MantidPythonInterface/kernel/Converters/WrapWithNumpy.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#include <boost/python/object.hpp>
#define PY_ARRAY_UNIQUE_SYMBOL KERNEL_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

//-----------------------------------------------------------------------------
// IPeakFunction definition
//-----------------------------------------------------------------------------
namespace Mantid {
namespace PythonInterface {
using Environment::callMethod;
using namespace boost::python;

/**
 * Construct the "wrapper" and stores the reference to the PyObject
 * @param self A reference to the calling Python object
 */
IPeakFunctionAdapter::IPeakFunctionAdapter(PyObject *self)
    : API::IPeakFunction(), IFunction1DAdapter(self) {}

/**
 */
double IPeakFunctionAdapter::centre() const {
  return callMethod<double>(getSelf(), "centre");
}

/**
 */
double IPeakFunctionAdapter::height() const {
  return callMethod<double>(getSelf(), "height");
}

/**
 * Called when the centre of the peak has been updated outside of the function
 * @param c The centre of the peak
 */
void IPeakFunctionAdapter::setCentre(const double c) {
  callMethod<void, double>(getSelf(), "setCentre", c);
}

/**
 * Called when the height of the peak has been updated outside of the function
 * @param h The new height of the peak
 */
void IPeakFunctionAdapter::setHeight(const double h) {
  callMethod<void, double>(getSelf(), "setHeight", h);
}

/// Calls Python fwhm method
double IPeakFunctionAdapter::fwhm() const {
  return callMethod<double>(getSelf(), "fwhm");
}

/**
 * Called when the width of the peak has been updated outside of the function
 * @param w The new width of the peak. The function should update its parameters
 * such that fwhm=w
 */
void IPeakFunctionAdapter::setFwhm(const double w) {
  return callMethod<void, double>(getSelf(), "setFwhm", w);
}

/**
 * Translates between the C++ signature & the Python signature and will be
 * called by Fit
 * @param out The 1D data array of size nData that stores the output values
 * @param xValues The input X values
 * @param nData The size of the two arrays
 */
void IPeakFunctionAdapter::functionLocal(double *out, const double *xValues,
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
      PyEval_CallMethod(getSelf(), "functionLocal", "(O)", xvals);
  Py_DECREF(xvals);
  if (PyErr_Occurred()) {
    Py_DECREF(result);
    throw Environment::PythonException();
  }

  PyArrayObject *nparray = reinterpret_cast<PyArrayObject *>(result);
  if (PyArray_TYPE(nparray) ==
      NPY_DOUBLE) // dtype matches so use memcpy for speed
  {
    std::memcpy(static_cast<void *>(out), PyArray_DATA(nparray),
                nData * sizeof(npy_double));
    Py_DECREF(result);
  } else {
    Py_DECREF(result);
    PyArray_Descr *dtype = PyArray_DESCR(nparray);
    PyObject *name = PyList_GetItem(dtype->names, 0);
    std::ostringstream os;
    os << "Unsupported numpy data type: '" << PyBytes_AsString(name)
       << "'. Currently only numpy.float64 is supported";
    throw std::runtime_error(os.str());
  }
}

/**
 * Python-type signature version of above so that users can call functionLocal
 * directly from Python on a factory
 * created object
 * @param xvals The input X values in read-only numpy array
 */
object
IPeakFunctionAdapter::functionLocal(const boost::python::object &xvals) const {
  return callMethod<object, object>(getSelf(), "functionLocal", xvals);
}

/**
 * Translates between the C++ signature & the Python signature and will be
 * called by Fit
 * @param out The Jacobian matrix storing the partial derivatives of the
 * function w.r.t to the parameters
 * @param xValues The input X values
 * @param nData The size of the two arrays
 */
void IPeakFunctionAdapter::functionDerivLocal(API::Jacobian *out,
                                              const double *xValues,
                                              const size_t nData) {
  using namespace Converters;
  // GIL must be held while numpy wrappers are destroyed as they access Python
  // state information
  Environment::GlobalInterpreterLock gil;

  Py_intptr_t dims[1] = {static_cast<Py_intptr_t>(nData)};
  PyObject *xvals =
      WrapReadOnly::apply<double>::createFromArray(xValues, 1, dims);
  PyObject *jacobian = boost::python::to_python_value<API::Jacobian *>()(out);

  // Deliberately avoids using the CallMethod wrappers. They lock the GIL again
  // and
  // will check for each function call whether the wrapped method exists. It
  // also avoid unnecessary construction of
  // boost::python::objects when using boost::python::call_method
  PyEval_CallMethod(getSelf(), "functionDerivLocal", "(OO)", xvals, jacobian);
  if (PyErr_Occurred())
    throw Environment::PythonException();
}

/**
 * Python-type signature version of above that can be called directly from
 * Python
 * @param xvals The input X values in read-only numpy array
 *  @param jacobian The Jacobian matrix storing the partial derivatives of the
 * function w.r.t to the parameters
 */
void IPeakFunctionAdapter::functionDerivLocal(
    const boost::python::object &xvals, boost::python::object &jacobian) {
  callMethod<void, object, object>(getSelf(), "functionDerivLocal", xvals,
                                   jacobian);
}
} // namespace PythonInterface
} // namespace Mantid
