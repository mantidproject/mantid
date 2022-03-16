// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "LookupTable.h"
#include "IGroup.h"
#include "MantidKernel/Logger.h"
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

Mantid::Kernel::Logger g_log("Reflectometry Lookup Table");
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

LookupTable::LookupTable(std::vector<LookupRow> rowsIn) : m_lookupRows(std::move(rowsIn)) {}

LookupTable::LookupTable(std::initializer_list<LookupRow> rowsIn) : m_lookupRows(rowsIn) {}

std::vector<LookupRow> const &LookupTable::rows() const { return m_lookupRows; }

boost::optional<LookupRow> LookupTable::findLookupRow(Row const &row, double tolerance) const {
  // First filter lookup rows by title, if the run has one
  auto lookupRows = searchByTitle(row);
  if (auto found = searchByTheta(lookupRows, row.theta(), tolerance)) {
    return found;
  }
  // If we didn't find an explicit regex that matches, then we allow the user to specify a lookup row with an empty
  // regex as a default, which falls back to matching all titles
  lookupRows = findEmptyRegexes();
  // Now filter by angle; it should be unique
  if (auto found = searchByTheta(lookupRows, row.theta(), tolerance)) {
    return found;
  }
  // If we didn't find a lookup row where theta matches, then we allow the user to specify a "wildcard" row
  // which will be used for everything where a specific match is not found
  auto result = findWildcardLookupRow();
  if (result) {
    g_log.warning(
        "No matching experiment settings found for " + boost::algorithm::join(row.runNumbers(), ", ") +
        ". Using wildcard row settings instead. You may wish to check that all lookup criteria on the Experiment "
        "Settings table are correct.");
    return result;
  }
  g_log.warning(
      "No matching experiment settings found for " + boost::algorithm::join(row.runNumbers(), ", ") +
      ". Using algorithm default settings instead. You may wish to check that all lookup criteria on the Experiment "
      "Settings table are correct.");
  return result;
}

boost::optional<LookupRow> LookupTable::searchByTheta(std::vector<LookupRow> lookupRows,
                                                      boost::optional<double> const &thetaAngle,
                                                      double tolerance) const {
  std::vector<LookupRow> matchingRows;
  auto predicate = [thetaAngle, tolerance](LookupRow const &candiate) -> bool {
    return !candiate.isWildcard() && equalWithinTolerance(*thetaAngle, candiate.thetaOrWildcard().get(), tolerance);
  };

  std::copy_if(lookupRows.cbegin(), lookupRows.cend(), std::back_inserter(matchingRows), predicate);

  if (matchingRows.empty())
    return boost::none;
  else if (matchingRows.size() == 1) {
    return matchingRows[0];
  } else {
    throw MultipleRowsFoundException("Multiple matching Experiment Setting rows");
  }
}

std::vector<LookupRow> LookupTable::findMatchingRegexes(std::string const &title) const {
  auto results = std::vector<LookupRow>();
  std::copy_if(m_lookupRows.cbegin(), m_lookupRows.cend(), std::back_inserter(results),
               [&title](auto const &candidate) {
                 return candidate.titleMatcher() && boost::regex_search(title, candidate.titleMatcher().get());
               });
  return results;
}

std::vector<LookupRow> LookupTable::findEmptyRegexes() const {
  auto results = std::vector<LookupRow>();
  std::copy_if(m_lookupRows.cbegin(), m_lookupRows.cend(), std::back_inserter(results),
               [](auto const &candidate) { return !candidate.titleMatcher(); });
  return results;
}

std::vector<LookupRow> LookupTable::searchByTitle(Row const &row) const {
  if (!row.getParent()) {
    return findMatchingRegexes("");
  }

  auto const &title = row.getParent()->name();
  auto results = findMatchingRegexes(title);
  return results;
}

boost::optional<LookupRow> LookupTable::findWildcardLookupRow() const {
  auto match = std::find_if(m_lookupRows.cbegin(), m_lookupRows.cend(),
                            [](LookupRow const &candidate) -> bool { return candidate.isWildcard(); });
  if (match == m_lookupRows.cend())
    return boost::none;
  else
    return *match;
}

size_t LookupTable::getIndex(const LookupRow &lookupRow) const {
  if (auto found = std::find(m_lookupRows.cbegin(), m_lookupRows.cend(), lookupRow); found != m_lookupRows.cend()) {
    auto index = std::distance(m_lookupRows.cbegin(), found);
    assert(index >= 0);
    return static_cast<size_t>(index);
  }
  throw std::out_of_range("Lookup row not found.");
}

std::vector<LookupRow::ValueArray> LookupTable::toValueArray() const {
  auto result = std::vector<LookupRow::ValueArray>();
  std::transform(m_lookupRows.cbegin(), m_lookupRows.cend(), std::back_inserter(result),
                 [](auto const &lookupRow) { return lookupRowToArray(lookupRow); });
  return result;
}

bool operator==(LookupTable const &lhs, LookupTable const &rhs) { return lhs.m_lookupRows == rhs.m_lookupRows; }
bool operator!=(LookupTable const &lhs, LookupTable const &rhs) { return !operator==(lhs, rhs); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
