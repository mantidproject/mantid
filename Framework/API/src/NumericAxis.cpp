// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/format.hpp>
#include <cmath>
#include <utility>

#include "MantidKernel/Logger.h"
namespace {
Mantid::Kernel::Logger g_log("NumericAxis");

class EqualWithinTolerance {
public:
  explicit EqualWithinTolerance(double tolerance) : m_tolerance(tolerance){};
  bool operator()(double a, double b) {
    if (std::isnan(a) && std::isnan(b))
      return true;
    if (std::isinf(a) && std::isinf(b))
      return true;
    return std::abs(a - b) <= m_tolerance;
  }

private:
  double m_tolerance;
};
} // namespace

namespace Mantid::API {

//------------------------------------------------------------------------------
// public members
//------------------------------------------------------------------------------

/** Returns the index of the value wrt bin edge representation of the axis
 * @param value A value to find in a bin
 * @return The index of the bin holding the value
 */
size_t NumericAxis::indexOfValue(const double value) const {
  return Mantid::Kernel::VectorHelper::indexOfValueFromCenters(m_values, value);
}

/** Constructor
 * @param length size of the numeric axis
 */
NumericAxis::NumericAxis(const std::size_t &length) : Axis(), m_values(length) {}

/**
 * Constructor taking a set of centre point values
 * @param centres A vector of values to assign to the axis
 */
NumericAxis::NumericAxis(std::vector<double> centres) : Axis(), m_values(std::move(centres)) {}

/** Virtual constructor
 *  @param parentWorkspace :: The workspace is not used in this implementation
 *  @returns A pointer to a copy of the NumericAxis on which the method is
 * called
 */
Axis *NumericAxis::clone(const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  return new NumericAxis(*this);
}

Axis *NumericAxis::clone(const std::size_t length, const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  auto newAxis = new NumericAxis(*this);
  newAxis->m_values.clear();
  newAxis->m_values.resize(length);
  return newAxis;
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored
 * (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double NumericAxis::operator()(const std::size_t &index, const std::size_t &verticalIndex) const {
  UNUSED_ARG(verticalIndex)
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1, "NumericAxis: Index out of range.");
  }

  return m_values[index];
}

/** Sets the axis value at a given position
 *  @param index :: The position along the axis for which to set the value
 *  @param value :: The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void NumericAxis::setValue(const std::size_t &index, const double &value) {
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1, "NumericAxis: Index out of range.");
  }

  m_values[index] = value;
}

/** Check if two NumericAxis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true if self and second axis are equal
 */
bool NumericAxis::operator==(const NumericAxis &axis2) const { return equalWithinTolerance(axis2, 1e-15); }

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true if self and second axis are equal
 */
bool NumericAxis::operator==(const Axis &axis2) const { return equalWithinTolerance(axis2, 1e-15); }

/** Check if two numeric axis are equivalent to a given tolerance
 *  @param axis2 :: Reference to the axis to compare to
 *  @param tolerance :: Tolerance to compare to
 *  @return true if self and second axis are equal
 */
bool NumericAxis::equalWithinTolerance(const Axis &axis2, const double tolerance) const {
  if (length() != axis2.length()) {
    return false;
  }
  const auto *spec2 = dynamic_cast<const NumericAxis *>(&axis2);
  if (!spec2) {
    return false;
  }
  // Check each value is within tolerance
  EqualWithinTolerance comparison(tolerance);
  return std::equal(m_values.begin(), m_values.end(), spec2->m_values.begin(), comparison);
}

/** Returns a text label which shows the value corresponding to a bin index.
 *  @param index :: The index of the bin, matches with the index of the value
 *  @return the formatted string
 */
std::string NumericAxis::label(const std::size_t &index) const { return formatLabel((*this)(index)); }

/**
 * @brief Formats a numeric label to a string
 * @param value: the numeric value of the label
 * @return formatted value as string
 */
std::string NumericAxis::formatLabel(const double value) const {
  std::string numberLabel = boost::str(boost::format("%.13f") % value);

  // Remove all zeros up to the decimal place or a non zero value
  auto it = numberLabel.end() - 1;
  for (; it != numberLabel.begin(); --it) {
    if (*it == '0') {
      it = numberLabel.erase(it);
    } else if (*it == '.') {
      it = numberLabel.erase(it);
      break;
    } else {
      break;
    }
  }

  return numberLabel;
}

/**
 * Create a set of bin boundaries from the centre point values
 * @returns A vector of bin boundaries
 */
std::vector<double> NumericAxis::createBinBoundaries() const {
  std::vector<double> result;
  result.reserve(m_values.size());
  Kernel::VectorHelper::convertToBinBoundary(m_values, result);
  return result;
}

/** Get a const reference to the vector of values in this axis
 *
 * @return the values vector
 */
const std::vector<double> &NumericAxis::getValues() const { return m_values; }

//------------------------------------------------------------------------------
// Protected members
//------------------------------------------------------------------------------

/**
 * Sets values vectors to zero size
 */
NumericAxis::NumericAxis() = default;

} // namespace Mantid::API
