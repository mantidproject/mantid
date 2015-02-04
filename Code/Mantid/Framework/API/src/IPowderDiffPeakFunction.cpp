//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidAPI/Jacobian.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"

#include <boost/lexical_cast.hpp>
#include <cmath>

const double IGNOREDCHANGE = 1.0E-9;

namespace Mantid {
namespace API {

int IPowderDiffPeakFunction::s_peakRadius = 5;

//----------------------------------------------------------------------------------------------
/** Constructor. Sets peak radius to the value of curvefitting.peakRadius
 * property
  */
IPowderDiffPeakFunction::IPowderDiffPeakFunction()
    : LATTICEINDEX(9999), HEIGHTINDEX(9999) {
  // Set peak's radius from configuration
  int peakRadius;
  if (Kernel::ConfigService::Instance().getValue("curvefitting.peakRadius",
                                                 peakRadius)) {
    if (peakRadius != s_peakRadius) {
      setPeakRadius(peakRadius);
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Desctructor
  */
IPowderDiffPeakFunction::~IPowderDiffPeakFunction() {}

//----------------------------------------------------------------------------------------------
/** Override setting parameter by parameter index
  * @param i :: parameter index in function;
  * @param value :: parameter name
  * @param explicitlySet ::
  */
void IPowderDiffPeakFunction::setParameter(size_t i, const double &value,
                                           bool explicitlySet) {
  double origparamvalue = getParameter(i);
  if (fabs(origparamvalue - value) > IGNOREDCHANGE) {
    m_hasNewParameterValue = true;
  }
  ParamFunction::setParameter(i, value, explicitlySet);

  return;
}

//----------------------------------------------------------------------------------------------
/** Overriding setting parameter by parameter name
  * @param name :: name of the parameter to set
  * @param value :: parameter name
  * @param explicitlySet ::
  */
void IPowderDiffPeakFunction::setParameter(const std::string &name,
                                           const double &value,
                                           bool explicitlySet) {
  double origparamvalue = getParameter(name);
  if (fabs(origparamvalue - value) > IGNOREDCHANGE) {
    m_hasNewParameterValue = true;
  }
  ParamFunction::setParameter(name, value, explicitlySet);

  return;
}

//----------------------------------------------------------------------------------------------
/** Get peak centre
  */
double IPowderDiffPeakFunction::centre() const {
  // Re-calcualte peak parameters if required
  if (m_hasNewParameterValue)
    calculateParameters(false);

  return m_centre;
}

//----------------------------------------------------------------------------------------------
/** Get peak height
double IPowderDiffPeakFunction::height() const
{
  return m_intensity;
}
*/

//----------------------------------------------------------------------------------------------
/**  Set peak height (intensity indeed)

void IPowderDiffPeakFunction::setHeight(const double h)
{
  m_intensity = h;

  return;
}
    */

//----------------------------------------------------------------------------------------------
/** Set peak height
 */
void IPowderDiffPeakFunction::setHeight(const double h) {
  setParameter(HEIGHTINDEX, h);

  return;
}

//----------------------------------------------------------------------------------------------
/** Get peak's height
  */
double IPowderDiffPeakFunction::height() const {
  double height = this->getParameter(HEIGHTINDEX);
  return height;
}

//----------------------------------------------------------------------------------------------
/**  Get peak's FWHM
  */
double IPowderDiffPeakFunction::fwhm() const {
  if (m_hasNewParameterValue)
    calculateParameters(false);

  return m_fwhm;
}

//----------------------------------------------------------------------------------------------
/** Get maximum value on a given set of data points
  */
double
IPowderDiffPeakFunction::getMaximumValue(const std::vector<double> &xValues,
                                         size_t &indexmax) const {
  // Calculate function with given data points
  std::vector<double> out(xValues.size(), 0.);
  function(out, xValues);

  // Find out the maximum value
  double max = -DBL_MAX;
  indexmax = 0;
  for (size_t i = 0; i < out.size(); ++i) {
    if (out[i] > max) {
      max = out[i];
      indexmax = i;
    }
  }

  return max;
}

//----------------------------------------------------------------------------------------------
/** Set Miller Indices for this peak
 */
void IPowderDiffPeakFunction::setMillerIndex(int h, int k, int l) {
  // Check validity and set flag
  if (mHKLSet) {
    // Throw exception if tried to reset the miller index
    std::stringstream errss;
    errss << "Profile function " << name() << "cannot have (HKL) reset.";
    throw std::runtime_error(errss.str());
  } else {
    // Set flag
    mHKLSet = true;
  }

  // Set value
  mH = static_cast<int>(h);
  mK = static_cast<int>(k);
  mL = static_cast<int>(l);

  // Check value valid or not
  if (mH * mH + mK * mK + mL * mL < 1.0E-8) {
    std::stringstream errmsg;
    errmsg << "H = K = L = 0 is not allowed";
    throw std::invalid_argument(errmsg.str());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Get Miller Index from this peak
 */
void IPowderDiffPeakFunction::getMillerIndex(int &h, int &k, int &l) {
  h = static_cast<int>(mH);
  k = static_cast<int>(mK);
  l = static_cast<int>(mL);

  return;
}

//----------------------------------------------------------------------------------------------
/** Set peak radius
  * @param r :: radius
  */
void IPowderDiffPeakFunction::setPeakRadius(const int &r) {
  if (r > 0) {
    s_peakRadius = r;

    // std::string setting = boost::lexical_cast<std::string>(r);
    // Kernel::ConfigService::Instance().setString("curvefitting.peakRadius",setting);
  }
}

//----------------------------------------------------------------------------------------------
/** Check whether a parameter is a profile parameter
  */
bool IPowderDiffPeakFunction::hasProfileParameter(std::string paramname) {
  std::vector<std::string>::iterator niter;
  niter = lower_bound(m_sortedProfileParameterNames.begin(),
                      m_sortedProfileParameterNames.end(), paramname);
  if (niter == m_sortedProfileParameterNames.end())
    return false;

  std::string candname = *niter;
  if (candname.compare(paramname))
    return false;

  return true;
}

//-------------------------  External Functions
//---------------------------------------------------
// FIXME : This is the same function used for ThermalNeutron... ...
/** Implementation of complex integral E_1
 */
std::complex<double> E1(std::complex<double> z) {
  std::complex<double> exp_e1;

  double rz = real(z);
  double az = abs(z);

  if (fabs(az) < 1.0E-8) {
    // If z = 0, then the result is infinity... diverge!
    std::complex<double> r(1.0E300, 0.0);
    exp_e1 = r;
  } else if (az <= 10.0 || (rz < 0.0 && az < 20.0)) {
    // Some interesting region, equal to integrate to infinity, converged
    // cout << "[DB] Type 1" << endl;

    std::complex<double> r(1.0, 0.0);
    exp_e1 = r;
    std::complex<double> cr = r;

    for (size_t k = 1; k <= 150; ++k) {
      double dk = double(k);
      cr = -cr * dk * z / ((dk + 1.0) * (dk + 1.0));
      exp_e1 += cr;
      if (abs(cr) < abs(exp_e1) * 1.0E-15) {
        // cr is converged to zero
        break;
      }
    } // ENDFOR k

    const double el = 0.5772156649015328;
    exp_e1 = -el - log(z) + (z * exp_e1);
  } else {
    // Rest of the region
    std::complex<double> ct0(0.0, 0.0);
    for (int k = 120; k > 0; --k) {
      std::complex<double> dk(double(k), 0.0);
      ct0 = dk / (10.0 + dk / (z + ct0));
    } // ENDFOR k

    exp_e1 = 1.0 / (z + ct0);
    exp_e1 = exp_e1 * exp(-z);
    if (rz < 0.0 && fabs(imag(z)) < 1.0E-10) {
      std::complex<double> u(0.0, 1.0);
      exp_e1 = exp_e1 - (M_PI * u);
    }
  }

  return exp_e1;
}

} // namespace API
} // namespace Mantid
