#include "MantidPythonInterface/api/FitFunctions/IFunction1DAdapter.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

#define PY_ARRAY_UNIQUE_SYMBOL API_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

//-----------------------------------------------------------------------------
// IFunction1D definition
//-----------------------------------------------------------------------------
namespace Mantid {
namespace PythonInterface {
using Environment::callMethodNoCheck;
using namespace boost::python;

/**
 * Construct the "wrapper" and stores the reference to the PyObject
 * @param self A reference to the calling Python object
 */
IFunction1DAdapter::IFunction1DAdapter(PyObject *self)
    : API::ParamFunction(), API::IFunction1D(),
      IFunctionAdapter(self, "function1D", "functionDeriv1D") {}

/**
 * Translates between the C++ signature & the Python signature called by Fit
 * @param out The 1D data array of size nData that stores the output values
 * @param xValues The input X values
 * @param nData The size of the two arrays
 */
void IFunction1DAdapter::function1D(double *out, const double *xValues,
                                    const size_t nData) const {
  evaluateFunction(out, xValues, nData);
}

/**
 * Python-type signature version of above to be called directly from Python
 * @param xvals The input X values in read-only numpy array
 */
boost::python::object
IFunction1DAdapter::function1D(const boost::python::object &xvals) const {
  return callMethodNoCheck<object, object>(getSelf(), "function1D", xvals);
}

/**
 * If a Python override exists then call that, otherwise call the base class
 * method
 * @param out The Jacobian matrix storing the partial derivatives of the
 * function w.r.t to the parameters
 * @param xValues The input X values
 * @param nData The size of the two arrays
 */
void IFunction1DAdapter::functionDeriv1D(API::Jacobian *out,
                                         const double *xValues,
                                         const size_t nData) {
  if (derivativeOverridden()) {
    evaluateDerivative(out, xValues, nData);
  } else {
    Base::functionDeriv1D(out, xValues, nData);
  }
}
} // namespace PythonInterface
} // namespace Mantid
