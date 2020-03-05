// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include <sstream>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

using namespace Kernel;

/// Constructor, takes an orientation matrix and lambda min/max.
HKLFilterWavelength::HKLFilterWavelength(const Kernel::DblMatrix &ub,
                                         double lambdaMin, double lambdaMax)
    : m_ub(ub), m_lambdaMin(lambdaMin), m_lambdaMax(lambdaMax) {
  checkProperLambdaRangeValues();
}

/// Returns a description for the filter.
std::string HKLFilterWavelength::getDescription() const noexcept {
  std::ostringstream strm;
  strm << "(" << m_lambdaMin << " <= lambda <= " << m_lambdaMax << ")";

  return strm.str();
}

/// Returns true if lambda of the reflection is within the limits.
bool HKLFilterWavelength::isAllowed(const Kernel::V3D &hkl) const noexcept {
  V3D q = m_ub * hkl;
  double lambda = (2.0 * q.Z()) / (q.norm2());

  return lambda >= m_lambdaMin && lambda <= m_lambdaMax;
}

/// Throws std::range_error exceptions if limits are <= 0 or max >= min.
void HKLFilterWavelength::checkProperLambdaRangeValues() const {
  if (m_lambdaMin <= 0.0) {
    throw std::range_error("LambdaMin cannot be <= 0.");
  }

  if (m_lambdaMax <= m_lambdaMin) {
    throw std::range_error("LambdaMax cannot be smaller than LambdaMin.");
  }
}

} // namespace Geometry
} // namespace Mantid
