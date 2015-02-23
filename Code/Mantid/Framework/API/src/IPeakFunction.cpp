//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/PeakFunctionIntegrator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <boost/lexical_cast.hpp>
#include <cmath>

namespace Mantid {
namespace API {

/** A Jacobian for individual functions
 */
class PartialJacobian1 : public Jacobian {
  Jacobian *m_J; ///< pointer to the overall Jacobian
  int m_iY0;     ///< offset in the overall Jacobian for a particular function
public:
  /** Constructor
   * @param J :: A pointer to the overall Jacobian
   * @param iY0 :: The data offset for a particular function
   */
  PartialJacobian1(Jacobian *J, int iY0) : m_J(J), m_iY0(iY0) {}
  /**
   * Overridden Jacobian::set(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   * @param value :: The derivative value
   */
  void set(size_t iY, size_t iP, double value) {
    m_J->set(m_iY0 + iY, iP, value);
  }
  /**
   * Overridden Jacobian::get(...).
   * @param iY :: The index of the data point
   * @param iP :: The parameter index of an individual function.
   */
  double get(size_t iY, size_t iP) { return m_J->get(m_iY0 + iY, iP); }
};

/// Default value for the peak radius
int IPeakFunction::s_peakRadius = 5;

/**
  * Constructor. Sets peak radius to the value of curvefitting.peakRadius
 * property
  */
IPeakFunction::IPeakFunction() {
  int peakRadius;
  if (Kernel::ConfigService::Instance().getValue("curvefitting.peakRadius",
                                                 peakRadius)) {
    if (peakRadius != s_peakRadius) {
      setPeakRadius(peakRadius);
    }
  }
}

/**
 * General implementation of the method for all peaks. Limits the peak
 * evaluation to
 * a certain number of FWHMs around the peak centre. The outside points are set
 * to 0.
 * Calls functionLocal() to compute the actual values
 * @param out :: Output function values
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IPeakFunction::function1D(double *out, const double *xValues,
                               const size_t nData) const {
  double c = this->centre();
  double dx = fabs(s_peakRadius * this->fwhm());
  int i0 = -1;
  int n = 0;
  for (size_t i = 0; i < nData; ++i) {
    if (fabs(xValues[i] - c) < dx) {
      if (i0 < 0)
        i0 = static_cast<int>(i);
      ++n;
    } else {
      out[i] = 0.0;
    }
  }
  if (i0 < 0 || n == 0)
    return;
  this->functionLocal(out + i0, xValues + i0, n);
}

/**
 * General implementation of the method for all peaks. Calculates derivatives
 * only
 * for a range of x values limited to a certain number of FWHM around the peak
 * centre.
 * For the points outside the range all derivatives are set to 0.
 * Calls functionDerivLocal() to compute the actual values
 * @param out :: Derivatives
 * @param xValues :: X values for data points
 * @param nData :: Number of data points
 */
void IPeakFunction::functionDeriv1D(Jacobian *out, const double *xValues,
                                    const size_t nData) {
  double c = this->centre();
  double dx = fabs(s_peakRadius * this->fwhm());
  int i0 = -1;
  int n = 0;
  for (size_t i = 0; i < nData; ++i) {
    if (fabs(xValues[i] - c) < dx) {
      if (i0 < 0)
        i0 = static_cast<int>(i);
      ++n;
    } else {
      for (size_t ip = 0; ip < this->nParams(); ++ip) {
        out->set(i, ip, 0.0);
      }
    }
  }
  if (i0 < 0 || n == 0)
    return;
  PartialJacobian1 J(out, i0);
  this->functionDerivLocal(&J, xValues + i0, n);
}

void IPeakFunction::setPeakRadius(const int &r) {
  if (r > 0) {
    s_peakRadius = r;
    std::string setting = boost::lexical_cast<std::string>(r);
    Kernel::ConfigService::Instance().setString("curvefitting.peakRadius",
                                                setting);
  }
}

/// Returns the integral intensity of the peak function, using the peak radius
/// to determine integration borders.
double IPeakFunction::intensity() const {
  double x0 = centre();
  double dx = fabs(s_peakRadius * fwhm());

  PeakFunctionIntegrator integrator;
  IntegrationResult result = integrator.integrate(*this, x0 - dx, x0 + dx);

  if (!result.success) {
    return 0.0;
  }

  return result.result;
}

/// Sets the integral intensity of the peak by adjusting the height.
void IPeakFunction::setIntensity(const double newIntensity) {
  double currentHeight = height();
  double currentIntensity = intensity();

  if (currentIntensity == 0.0) {
    // Try to set a different height first.
    setHeight(2.0);

    currentHeight = height();
    currentIntensity = intensity();

    // If the current intensity is still 0, there's nothing left to do.
    if (currentIntensity == 0.0) {
      throw std::invalid_argument(
          "Cannot set new intensity, not enough information available.");
    }
  }

  setHeight(newIntensity / currentIntensity * currentHeight);
}


std::string IPeakFunction::getCentreParameterName() const
{
    FunctionParameterDecorator_sptr fn = boost::make_shared<SpecialParameterFunction>();
    fn->setDecoratedFunction(this->name());

    FunctionDomain1DVector domain(std::vector<double>(4, 0.0));
}


} // namespace API
} // namespace Mantid
