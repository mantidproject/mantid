//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/PeakParametersNumeric.h"
#include "MantidCurveFitting/ChebfunBase.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/make_shared.hpp>
#include <cmath>
#include <limits>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

/// Specify a name of a parameter that scales in sync with the peak height.
/// Multiplying this parameter by a factor changes the height by the same
/// factor.
/// @param parName :: A parameter name
void PeakParametersNumeric::defineHeightParameter(const std::string &parName) {
  m_heightIndex = parameterIndex(parName);
}

/// Specify a name of a parameter that shifts in sync with the peak centre.
/// Adding a value to this parameter moves the centre by the same amount.
/// @param parName :: A parameter name
void PeakParametersNumeric::defineCentreParameter(const std::string &parName) {
  m_centreIndex = parameterIndex(parName);
}

/// Add a name of a parameter that affects the peak width. There can be more
/// than one such parameter.
/// @param parName :: A parameter name
/// @param wType :: The kind of dependency between the width and this parameter.
///       If the width needs to be scaled by factor f
///         - Linear parameter is scaled by the same factor f,
///         - Square parameter is scaled by f^2
///         - Inverse parameter is scaled by 1/f.
void PeakParametersNumeric::defineWidthParameter(const std::string &parName,
                                                 WidthParamType wType) {
  m_widthIndices.push_back(parameterIndex(parName));
  m_widthParTypes.push_back(wType);
}

/// Calculate the peak height as the extreme value.
double PeakParametersNumeric::height() const {
  updateCache();
  return m_height;
}

/// Set new height of the peak.
/// @param h :: New value for the height.
void PeakParametersNumeric::setHeight(const double h) {
  double h0 = height();
  if (h0 == 0.0) {
    setParameter(m_heightIndex, 1e-6);
    h0 = height();
  }
  double parValue = getParameter(m_heightIndex);
  parValue *= h / h0;
  if (parValue <= 0.0) {
    parValue = 1e-6;
  }
  if (boost::math::isnan(parValue) || boost::math::isinf(parValue)) {
    parValue = std::numeric_limits<double>::max() / 2;
  }
  setParameter(m_heightIndex, parValue);
}

/// Calculate the peak centre as the extreme point.
double PeakParametersNumeric::centre() const {
  updateCache();
  return m_centre;
}

/// Set new centre position.
/// @param c :: New centre value.
void PeakParametersNumeric::setCentre(const double c) {
  double c0 = centre();
  double dc = c - c0;
  double x0 = getParameter(m_centreIndex) + dc;
  setParameter(m_centreIndex, x0);
}

/// Get the peak width as the distance between the two points
/// of half-maximum on both sides of the centre.
double PeakParametersNumeric::fwhm() const {
  updateCache();
  return m_width;
}

/// Set new peak width by scaling parameters specified by calls
/// to defineWidthParameter(...).
/// @param w :: New value for the width.
void PeakParametersNumeric::setFwhm(const double w) {
  double wOld = fwhm();
  double factor = w / wOld;
  for (size_t i = 0; i < m_widthIndices.size(); ++i) {
    size_t index = m_widthIndices[i];
    double value = getParameter(index);
    switch (m_widthParTypes[i]) {
    case Square:
      value *= factor * factor;
      break;
    case Inverse:
      value /= factor;
      break;
    case Linear:
    default:
      value *= factor;
    }
    setParameter(index, value);
  }
}

/// Calculate function value for a single argument.
/// @param x :: Function argument.
double PeakParametersNumeric::operator()(double x) const {
  double y = 0.0;
  function1D(&y, &x, 1);
  return y;
}

/// Override the base class method to set the dirty flag when any
/// of the parameters changes.
void PeakParametersNumeric::setParameter(size_t i, const double &value,
                                         bool explicitlySet) {
  IPeakFunction::setParameter(i, value, explicitlySet);
  m_invalidateCache = true;
}

/// Make function approximator ChebfunBase to approximate the peak
/// on an interval.
/// @param start :: Start of the interval.
/// @param end :: End of the interval.
/// @param p :: Peak values at pointes defined by ChebfunBase.
/// @param a :: Chebyshev expansion coefficients.
boost::shared_ptr<ChebfunBase>
PeakParametersNumeric::makeBase(double start, double end,
                                std::vector<double> &p,
                                std::vector<double> &a) const {
  double tolerance = 1e-15;
  ChebfunBase_sptr base;
  // Run bestFit with decreasing tolerance until approximation succeeds
  // Approximation of high accuracy can fail in cases of too sharp peaks
  // or too large extents.
  while (tolerance < 1.0) {
    base = ChebfunBase::bestFit(start, end, *this, p, a, 0.0, tolerance, 100);
    if (base)
      return base;
    tolerance *= 100;
  }
  // If all failed create an approximation with whatever accuracy
  // we can get.
  base = boost::make_shared<ChebfunBase>(8, start, end);
  p = base->fit(*this);
  a = base->calcA(p);
  return base;
}

/// Calculate the centre, peak and width if dirty flag has been raised.
void PeakParametersNumeric::updateCache() const {
  if (!m_invalidateCache)
    return;
  m_invalidateCache = false;

  // Get an interval for the approximation
  const auto interval = getExtent();
  double start = interval.first;
  double end = interval.second;

  // Containers for approximation's values
  std::vector<double> a, p, ad;

  bool baseBuilt = false;

  for (size_t iter = 0; iter < 2; ++iter) {
    baseBuilt = true;

    m_base = makeBase(start, end, p, a);
    m_base->derivative(a, ad);
    // Find the root(s) of the derivative which must give peak centre
    auto roots = m_base->roots(ad);

    // If approximation is bad there can be 0 or more than 1 roots
    if (roots.empty()) {
      // set the centre in the middle of the interval
      m_centre = (start + end) / 2;
    } else if (roots.size() == 1) {
      // this has to be the correct value
      m_centre = roots[0];
    } else {
      // if approximation ascillates find the root with the highest value
      double maxVal = 0.0;
      size_t iMax = 0;
      for (size_t i = 0; i < roots.size(); ++i) {
        double d = fabs((*this)(roots[i]));
        if (d > maxVal) {
          maxVal = d;
          iMax = i;
        }
      }
      m_centre = roots[iMax];
    }

    // height is peak's value at the centre
    m_height = (*this)(m_centre);

    // At this point we can check if getExtent() gave us a good interval.
    // Check the peak values at the ends of the interval are below a certain
    // fraction of the height.
    double h = fabs(m_height) / 8;
    double dStart = m_centre - start;
    while (fabs((*this)(start)) > h) {
      start -= dStart;
      baseBuilt = false;
    }
    double dEnd = end - m_centre;
    while (fabs((*this)(end)) > h) {
      end += dEnd;
      baseBuilt = false;
    }

    // If the interval turned out to be too small baseBuilt is false
    // and we make another iteration
    if (baseBuilt)
      break;
  }

  // The fastest way of shifting the approximation down the y-axis by height/2
  a[0] -= m_height / 2;

  // Now the roots should give the points of half-maximum
  auto roots = m_base->roots(a);

  // If the approximation isn't perfect consider different possibilities
  if (roots.empty()) {
    m_width = (end - start) / 2;
  } else if (roots.size() == 1) {
    m_width = fabs((end + start) / 2 - roots[0]);
  } else {
    m_width = fabs(roots[1] - roots[0]);
  }
}

} // namespace CurveFitting
} // namespace Mantid
