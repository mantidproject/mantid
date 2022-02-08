// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "LookupRowFinder.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupRowFinder::LookupRowFinder(const LookupTable &table) : m_lookupTable(table) {}

LookupRow const *LookupRowFinder::operator()(const boost::optional<double> &thetaAngle, double tolerance) const {
  LookupTable::const_iterator match;
  if (thetaAngle) {
    match = std::find_if(
        m_lookupTable.cbegin(), m_lookupTable.cend(), [thetaAngle, tolerance](LookupRow const &candiate) -> bool {
          return !candiate.isWildcard() && std::abs(*thetaAngle - candiate.thetaOrWildcard().get()) <= tolerance;
        });
  } else {
    match = std::find_if(m_lookupTable.cbegin(), m_lookupTable.cend(),
                         [](LookupRow const &candidate) -> bool { return candidate.isWildcard(); });
  }

  if (match != m_lookupTable.cend()) {
    return &(*match);
  } else if (thetaAngle) {
    // Try again without a specific angle i.e. look for a wildcard row
    return this->operator()(boost::none, tolerance);
  } else {
    return nullptr;
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry