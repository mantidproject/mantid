// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "LookupRowFinder.h"
#include "IGroup.h"
#include "Row.h"
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

boost::optional<LookupRow> LookupRowFinder::operator()(Row const &row, double tolerance) const {
  if (row.theta()) {
    // First filter lookup rows by title, if the run has one
    auto lookupRows = searchByTitle(row);

    if (lookupRows.empty()) {
      // If we didn't find an explicit regex that matches, then we allow the user to specify a lookup row with an empty
      // regex as a default, which falls back to matching all titles
      lookupRows = findEmptyRegexes();
    }

    // Now filter by angle; it should be unique
    if (auto found = searchByTheta(lookupRows, row.theta(), tolerance)) {
      return found;
    }
  }
  // No theta found/provided, look for wildcards
  return findWildcardLookupRow();
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

std::vector<LookupRow> LookupRowFinder::findMatchingRegexes(std::string const &title) const {
  auto results = std::vector<LookupRow>();
  std::copy_if(m_lookupTable.cbegin(), m_lookupTable.cend(), std::back_inserter(results),
               [&title](auto const &candidate) {
                 return candidate.titleMatcher() && boost::regex_search(title, candidate.titleMatcher().get());
               });
  return results;
}

std::vector<LookupRow> LookupRowFinder::findEmptyRegexes() const {
  auto results = std::vector<LookupRow>();
  std::copy_if(m_lookupTable.cbegin(), m_lookupTable.cend(), std::back_inserter(results),
               [](auto const &candidate) { return !candidate.titleMatcher(); });
  return results;
}

std::vector<LookupRow> LookupRowFinder::searchByTitle(Row const &row) const {
  if (!row.getParent() || row.getParent()->name().empty()) {
    // No titles for us to check against, so skip filtering
    return m_lookupTable;
  }

  auto const &title = row.getParent()->name();
  auto results = findMatchingRegexes(title);
  return results;
}

boost::optional<LookupRow> LookupRowFinder::findWildcardLookupRow() const {
  auto match = std::find_if(m_lookupTable.cbegin(), m_lookupTable.cend(),
                            [](LookupRow const &candidate) -> bool { return candidate.isWildcard(); });
  if (match == m_lookupTable.cend())
    return boost::none;
  else
    return *match;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry