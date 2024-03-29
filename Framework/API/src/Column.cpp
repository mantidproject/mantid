// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "MantidAPI/Column.h"
#include "MantidKernel/Logger.h"

namespace Mantid::API {

namespace {
/// static logger object
Kernel::Logger g_log("Column");
} // namespace

template <> bool Column::isType<bool>() const { return isBool(); }

/// Set plot type where
/// None = 0 (means it has specifically been set to 'no plot type')
/// NotSet = -1000 (this is the default and means plot style has not been set)
/// X = 1, Y = 2, Z = 3, xErr = 4, yErr = 5, Label = 6
/// @param t plot type as defined above
void Column::setPlotType(int t) {
  if (t == -1000 || t == 0 || t == 1 || t == 2 || t == 3 || t == 4 || t == 5 || t == 6)
    m_plotType = t;
  else {
    g_log.error() << "Cannot set plot of column to " << t << " . Ignore this attempt.\n";
  }
}

void Column::setLinkedYCol(const int yCol) { m_linkedYCol = yCol; }

/**
 * No implementation by default.
 */
void Column::sortIndex(bool /*unused*/, size_t /*unused*/, size_t /*unused*/, std::vector<size_t> & /*unused*/,
                       std::vector<std::pair<size_t, size_t>> & /*unused*/) const {
  throw std::runtime_error("Cannot sort column of type " + m_type);
}

/**
 * No implementation by default.
 */
void Column::sortValues(const std::vector<size_t> & /*unused*/) {
  throw std::runtime_error("Cannot sort column of type " + m_type);
}

std::ostream &operator<<(std::ostream &s, const API::Boolean &b) {
  s << (b.value ? "true" : "false");
  return s;
}

/**
 * Translate text into a Boolean.
 */
std::istream &operator>>(std::istream &istr, API::Boolean &b) {
  std::string buff;
  istr >> buff;
  std::transform(buff.begin(), buff.end(), buff.begin(), toupper);
  if (buff == "TRUE" || buff == "1" || buff == "OK" || buff == "YES" || buff == "ON") {
    b = true;
  } else {
    b = false;
  }
  return istr;
}

} // namespace Mantid::API
