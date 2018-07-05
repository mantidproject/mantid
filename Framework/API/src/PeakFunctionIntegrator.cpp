#include "MantidAPI/PeakFunctionIntegrator.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "gsl/gsl_errno.h"
#include <iomanip>

namespace Mantid {
namespace API {

/** Constructor with required relative precision argument. The default is 1e-8.
 *  See also PeakFunctionIntegrator::setRequiredRelativePrecision.
 *
 * @param requiredRelativePrecision :: Desired relative precision of the
 * integral estimations.
 */
PeakFunctionIntegrator::PeakFunctionIntegrator(double requiredRelativePrecision)
    : m_integrationWorkspace(gsl_integration_workspace_alloc(1000)),
      m_relativePrecision(requiredRelativePrecision) {
  /* Error handling is disabled, so error-codes are associated to the
   * integration result
   * and have to be checked.
   */
  gsl_set_error_handler_off();
}

PeakFunctionIntegrator::~PeakFunctionIntegrator() {
  gsl_integration_workspace_free(m_integrationWorkspace);
}

/** This method sets the desired numerical relative precision that's passed on
 *  to the GSL integration-routines.
 *
 *  @param newPrecision :: Desired relative precision for integrations.
 */
void PeakFunctionIntegrator::setRequiredRelativePrecision(double newPrecision) {
  m_relativePrecision = newPrecision;
}

/** Returns the currently set precision
 */
double PeakFunctionIntegrator::requiredRelativePrecision() const {
  return m_relativePrecision;
}

/** Integration of peak function on the interval [-Inf, +Inf]. Internally,
 *  gsl_integration_qagi is used for this. The results are returned as
 *  IntegrationResult-struct, which contains the approximation of the integral
 *  along with other information such as an error estimate (absolute).
 *
 *  @param peakFunction :: Peak function to integrate.
 */
IntegrationResult PeakFunctionIntegrator::integrateInfinity(
    const IPeakFunction &peakFunction) const {
  IntegrationResult result;

  gsl_function f = getGSLFunction(peakFunction);

  result.errorCode = gsl_integration_qagi(&f, 0, m_relativePrecision, 1000,
                                          m_integrationWorkspace,
                                          &result.result, &result.error);
  result.success = (result.errorCode == GSL_SUCCESS);
  result.intervals = m_integrationWorkspace->size;

  return result;
}

/** Integration of peak function on the interval [a, +Inf]. Internally,
 *  gsl_integration_qagiu is used for this.
 *
 *  @param peakFunction :: Peak function to integrate.
 *  @param lowerLimit :: Lower limit of the integration.
 */
IntegrationResult PeakFunctionIntegrator::integratePositiveInfinity(
    const IPeakFunction &peakFunction, double lowerLimit) const {
  IntegrationResult result;

  gsl_function f = getGSLFunction(peakFunction);

  result.errorCode = gsl_integration_qagiu(
      &f, lowerLimit, 0, m_relativePrecision, 1000, m_integrationWorkspace,
      &result.result, &result.error);
  result.success = (result.errorCode == GSL_SUCCESS);
  result.intervals = m_integrationWorkspace->size;

  return result;
}

/** Integration of peak function on the interval [-Inf, b]. Internally,
 *  gsl_integration_qagil is used for this.
 *
 *  @param peakFunction :: Peak function to integrate.
 *  @param upperLimit :: Upper limit of the integration.
 */
IntegrationResult PeakFunctionIntegrator::integrateNegativeInfinity(
    const IPeakFunction &peakFunction, double upperLimit) const {
  IntegrationResult result;

  gsl_function f = getGSLFunction(peakFunction);

  result.errorCode = gsl_integration_qagil(
      &f, upperLimit, 0, m_relativePrecision, 1000, m_integrationWorkspace,
      &result.result, &result.error);
  result.success = (result.errorCode == GSL_SUCCESS);
  result.intervals = m_integrationWorkspace->size;

  return result;
}

/** Integration of peak function on the interval [a, b]. Internally,
 *  gsl_integration_qags is used for this.
 *
 *  @param peakFunction :: Peak function to integrate.
 *  @param lowerLimit :: Lower limit of the integration.
 *  @param upperLimit :: Upper limit of the integration.
 */
IntegrationResult
PeakFunctionIntegrator::integrate(const IPeakFunction &peakFunction,
                                  double lowerLimit, double upperLimit) const {
  IntegrationResult result;

  gsl_function f = getGSLFunction(peakFunction);

  result.errorCode = gsl_integration_qags(
      &f, lowerLimit, upperLimit, 0, m_relativePrecision, 1000,
      m_integrationWorkspace, &result.result, &result.error);
  result.success = (result.errorCode == GSL_SUCCESS);
  result.intervals = m_integrationWorkspace->size;

  return result;
}

/** Method that wraps an IPeakFunction for use with GSL functions.
 *
 *  @param peakFunction :: Peak function to wrap.
 */
gsl_function PeakFunctionIntegrator::getGSLFunction(
    const IPeakFunction &peakFunction) const {
  gsl_function f;
  f.function = &Mantid::API::gsl_peak_wrapper;
  f.params =
      reinterpret_cast<void *>(&const_cast<IPeakFunction &>(peakFunction));

  return f;
}

double gsl_peak_wrapper(double x, void *parameters) {
  IPeakFunction *peakFunction = reinterpret_cast<IPeakFunction *>(parameters);

  if (!peakFunction) {
    throw std::runtime_error(
        "Cannot process NULL-pointer in gsl_peak_wrapper.");
  }

  double y;

  /* For the integration to work properly, functionLocal has to be used instead
   * of the more general function-method due to the fact that the overriden
   * function-method
   * in IPeakFunction cuts off at some point. For slowly decaying peak functions
   * such as Lorentzians, this is introduces large deviations for integrations
   * from -Inf to +Inf.
   */
  peakFunction->functionLocal(&y, &x, 1);

  return y;
}
} // namespace API
} // namespace Mantid
