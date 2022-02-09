// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "LookupRowFinder.h"
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <cmath>
#include <vector>

namespace {
constexpr double EPSILON = std::numeric_limits<double>::epsilon();

bool equalWithinTolerance(double val1, double val2, double tolerance) {
  return std::abs(val1 - val2) <= (tolerance + EPSILON);
}
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupRowFinder::LookupRowFinder(LookupTable const &table) : m_lookupTable(table) {}

// TODO pass in row rather than unpacking at call-site
boost::optional<LookupRow> LookupRowFinder::operator()(boost::optional<double> const &thetaAngle, double tolerance,
                                                       std::string const &title) const {
  if (thetaAngle) {
    // First filter lookup rows by title, if the run has one
    auto lookupRows = title.empty() ? m_lookupTable : searchByTitle(title);
    // Now filter by angle; it should be unique
    if (auto found = searchByTheta(lookupRows, thetaAngle, tolerance)) {
      return found;
    }
  }
  // No theta found/provided, look for wildcards
  return searchForWildcard();
}

boost::optional<LookupRow> LookupRowFinder::searchByTheta(std::vector<LookupRow> lookupRows,
                                                          boost::optional<double> const &thetaAngle,
                                                          double tolerance) const {
  // TODO We may get multiple matches if the title matches multiple regexes. If one regex is empty then we
  // can discard it. If we get multiple non-empty regex matches it is an error.
  auto match =
      std::find_if(lookupRows.cbegin(), lookupRows.cend(), [thetaAngle, tolerance](LookupRow const &candiate) -> bool {
        return !candiate.isWildcard() && equalWithinTolerance(*thetaAngle, candiate.thetaOrWildcard().get(), tolerance);
      });
  if (match == lookupRows.cend())
    return boost::none;
  else
    return *match;
}

std::vector<LookupRow> LookupRowFinder::searchByTitle(std::string const &title) const {
  auto results = std::vector<LookupRow>();
  std::copy_if(
      m_lookupTable.cbegin(), m_lookupTable.cend(), std::back_inserter(results), [&title](auto const &candidate) {
        return candidate.titleMatcher().has_value() && boost::regex_search(title, candidate.titleMatcher().get());
      });
  return results;
}

boost::optional<LookupRow> LookupRowFinder::searchForWildcard() const {
  auto match = std::find_if(m_lookupTable.cbegin(), m_lookupTable.cend(),
                            [](LookupRow const &candidate) -> bool { return candidate.isWildcard(); });
  if (match == m_lookupTable.cend())
    return boost::none;
  else
    return *match;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry