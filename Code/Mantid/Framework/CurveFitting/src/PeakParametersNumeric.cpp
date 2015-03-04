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

void PeakParametersNumeric::defineHeightParameter(const std::string &parName) {
  m_heightIndex = parameterIndex(parName);
}

void PeakParametersNumeric::defineCentreParameter(const std::string &parName) {
  m_centreIndex = parameterIndex(parName);
}

void PeakParametersNumeric::defineWidthParameter(const std::string &parName,
                                                 WidthParamType wType) {
  m_widthIndices.push_back(parameterIndex(parName));
  m_widthParTypes.push_back(wType);
}

/**
 * Get approximate height of the peak: function value at X0.
 */
double PeakParametersNumeric::height() const {
  updateCache();
  return m_height;
}

/**
 * Set new height of the peak. This method does this approximately.
 * @param h :: New value for the height.
 */
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

double PeakParametersNumeric::centre() const {
  updateCache();
  return m_centre;
}

void PeakParametersNumeric::setCentre(const double c) {
  double c0 = centre();
  double dc = c - c0;
  double x0 = getParameter(m_centreIndex) + dc;
  setParameter(m_centreIndex, x0);
}

/**
 * Get approximate peak width.
 */
double PeakParametersNumeric::fwhm() const {
  updateCache();
  return m_width;
}

/**
 * Set new peak width approximately.
 * @param w :: New value for the width.
 */
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

/**
 * Calculate function value for a single argument.
 */
double PeakParametersNumeric::operator()(double x) const {
  double y = 0.0;
  function1D(&y, &x, 1);
  return y;
}

void PeakParametersNumeric::setParameter(size_t i, const double &value,
                                         bool explicitlySet) {
  IPeakFunction::setParameter(i, value, explicitlySet);
  m_invalidateCache = true;
}

boost::shared_ptr<ChebfunBase>
PeakParametersNumeric::makeBase(double start, double end,
                                std::vector<double> &p,
                                std::vector<double> &a) const {
  double tolerance = 1e-15;
  ChebfunBase_sptr base;
  while (tolerance < 1.0) {
    base = ChebfunBase::bestFit(start, end, *this, p, a, 0.0, tolerance, 100);
    if (base)
      return base;
    tolerance *= 100;
  }
  base = boost::make_shared<ChebfunBase>(8, start, end);
  p = base->fit(*this);
  a = base->calcA(p);
  return base;
}

void PeakParametersNumeric::updateCache() const {
  if (!m_invalidateCache)
    return;
  m_invalidateCache = false;

  const auto interval = getExtent();
  double start = interval.first;
  double end = interval.second;

  std::vector<double> a, p, ad;

  bool baseBuilt = false;

  for (size_t iter = 0; iter < 2; ++iter) {
    baseBuilt = true;
    m_base = makeBase(start, end, p, a);

    m_base->derivative(a, ad);
    auto roots = m_base->roots(ad);

    if (roots.empty()) {
      m_centre = (start + end) / 2;
    } else if (roots.size() == 1) {
      m_centre = roots[0];
    } else {
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

    m_height = (*this)(m_centre);

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

    if (baseBuilt)
      break;
  }

  a[0] -= m_height / 2;

  auto roots = m_base->roots(a);

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
