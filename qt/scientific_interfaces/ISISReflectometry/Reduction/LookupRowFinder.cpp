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
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupRowFinder::LookupRowFinder(const LookupTable &table) : m_lookupTable(table) {}

LookupRow const *LookupRowFinder::operator()(const boost::optional<double> &thetaAngle, double tolerance) const {
  auto match = m_lookupTable.cend();
  if (thetaAngle) {
    if (const auto *found = searchByTheta(thetaAngle, tolerance)) {
      return found;
    }
  } else {
    match = std::find_if(m_lookupTable.cbegin(), m_lookupTable.cend(),
                         [](LookupRow const &candidate) -> bool { return candidate.isWildcard(); });
  }

  if (match != m_lookupTable.cend()) {
    return &(*match);
  } else if (thetaAngle) {
    // Try again without a specific angle i.e. look for a wildcard row
    // Wildcard branch
    return this->operator()(boost::none, tolerance);
  } else {
    return nullptr;
  }
}

LookupRow const *LookupRowFinder::searchByTheta(const boost::optional<double> &thetaAngle, double tolerance) const {
  auto match = std::find_if(m_lookupTable.cbegin(), m_lookupTable.cend(),
                            [thetaAngle, tolerance](LookupRow const &candiate) -> bool {
                              return !candiate.isWildcard() &&
                                     std::abs(*thetaAngle - candiate.thetaOrWildcard().get()) <= (tolerance + EPSILON);
                            });
  return match == m_lookupTable.cend() ? nullptr : &(*match);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry