// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "LookupRowFinder.h"
#include <cmath>

namespace {
constexpr double EPSILON = std::numeric_limits<double>::epsilon();

bool equalWithinTolerance(double val1, double val2, double tolerance) {
  return std::abs(val1 - val2) <= (tolerance + EPSILON);
}
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupRowFinder::LookupRowFinder(const LookupTable &table) : m_lookupTable(table) {}

LookupRow const *LookupRowFinder::operator()(const boost::optional<double> &thetaAngle, double tolerance) const {
  if (thetaAngle) {
    if (const auto *found = searchByTheta(thetaAngle, tolerance)) {
      return found;
    }
  }
  // No theta found/provided, look for wildcards
  return searchForWildcard();
}

LookupRow const *LookupRowFinder::searchByTheta(const boost::optional<double> &thetaAngle, double tolerance) const {
  auto match = std::find_if(
      m_lookupTable.cbegin(), m_lookupTable.cend(), [thetaAngle, tolerance](LookupRow const &candiate) -> bool {
        return !candiate.isWildcard() && equalWithinTolerance(*thetaAngle, candiate.thetaOrWildcard().get(), tolerance);
      });
  return match == m_lookupTable.cend() ? nullptr : &(*match);
}

LookupRow const *LookupRowFinder::searchForWildcard() const {
  auto match = std::find_if(m_lookupTable.cbegin(), m_lookupTable.cend(),
                            [](LookupRow const &candidate) -> bool { return candidate.isWildcard(); });
  return match == m_lookupTable.cend() ? nullptr : &(*match);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry