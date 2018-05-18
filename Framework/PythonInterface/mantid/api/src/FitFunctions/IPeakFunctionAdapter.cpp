#include "MantidPythonInterface/api/FitFunctions/IPeakFunctionAdapter.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"

//-----------------------------------------------------------------------------
// IPeakFunction definition
//-----------------------------------------------------------------------------
namespace Mantid {
namespace PythonInterface {
using Environment::callMethodNoCheck;
using namespace boost::python;

/**
 * Construct the "wrapper" and stores the reference to the PyObject
 * @param self A reference to the calling Python object
 */
IPeakFunctionAdapter::IPeakFunctionAdapter(PyObject *self)
    : IFunctionAdapter(self, "functionLocal", "functionDerivLocal") {}

/**
 */
double IPeakFunctionAdapter::centre() const {
  return callMethodNoCheck<double>(getSelf(), "centre");
}

/**
 */
double IPeakFunctionAdapter::height() const {
  return callMethodNoCheck<double>(getSelf(), "height");
}

/**
 * Called when the centre of the peak has been updated outside of the function
 * @param c The centre of the peak
 */
void IPeakFunctionAdapter::setCentre(const double c) {
  callMethodNoCheck<void, double>(getSelf(), "setCentre", c);
}

/**
 * Called when the height of the peak has been updated outside of the function
 * @param h The new height of the peak
 */
void IPeakFunctionAdapter::setHeight(const double h) {
  callMethodNoCheck<void, double>(getSelf(), "setHeight", h);
}

/// Calls Python fwhm method
double IPeakFunctionAdapter::fwhm() const {
  return callMethodNoCheck<double>(getSelf(), "fwhm");
}

/**
 * Called when the width of the peak has been updated outside of the function
 * @param w The new width of the peak. The function should update its parameters
 * such that fwhm=w
 */
void IPeakFunctionAdapter::setFwhm(const double w) {
  return callMethodNoCheck<void, double>(getSelf(), "setFwhm", w);
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
  evaluateFunction(out, xValues, nData);
}

/**
 * Python-type signature version of above so that users can call functionLocal
 * directly from Python on a factory
 * created object
 * @param xvals The input X values in read-only numpy array
 */
object
IPeakFunctionAdapter::functionLocal(const boost::python::object &xvals) const {
  return callMethodNoCheck<object, object>(getSelf(), "functionLocal", xvals);
}

/**
 * Translates between the C++ signature & the Python signature and will be
 * called by Fit
 * @param jacobian The Jacobian matrix storing the partial derivatives of the
 * function w.r.t to the parameters
 * @param xValues The input X values
 * @param nData The size of the two arrays
 */
void IPeakFunctionAdapter::functionDerivLocal(API::Jacobian *jacobian,
                                              const double *xValues,
                                              const size_t nData) {
  if (derivativeOverridden()) {
    evaluateDerivative(jacobian, xValues, nData);
  } else {
    Base::functionDerivLocal(jacobian, xValues, nData);
  }
}

/**
 * Python-type signature version of above that can be called directly from
 * Python
 * @param xvals The input X values in read-only numpy array
 * @param jacobian The Jacobian matrix storing the partial derivatives of the
 * function w.r.t to the parameters
 */
void IPeakFunctionAdapter::functionDerivLocal(
    const boost::python::object &xvals, boost::python::object &jacobian) {
  callMethodNoCheck<void, object, object>(getSelf(), "functionDerivLocal",
                                          xvals, jacobian);
}
} // namespace PythonInterface
} // namespace Mantid
