//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/format.hpp>

#include "MantidKernel/Logger.h"
namespace {
Mantid::Kernel::Logger g_log("NumericAxis");

// For variable tolerance comparison for axis values
double g_tolerance;
bool withinTolerance(double a, double b) {
  return std::abs(a - b) <= g_tolerance;
}
}

namespace Mantid {
namespace API {

//------------------------------------------------------------------------------
// static members
//------------------------------------------------------------------------------

/**
 * @param value A value to find in a bin
 * @param edges A list of edges that define the bins
 * @return The index of the bin holding the value
 */
size_t NumericAxis::indexOfValue(const double value,
                                 const std::vector<double> &edges) {
  if (value < edges.front()) {
    throw std::out_of_range(
        "NumericAxis::indexOfValue() - Input lower than first value.");
  }
  auto it = std::lower_bound(edges.begin(), edges.end(), value);
  if (it == edges.end()) {
    // Out of range
    throw std::out_of_range(
        "NumericAxis::indexOfValue() - Input value out of range");
  }
  // index of closest edge above value is distance of iterator from start
  size_t edgeIndex = std::distance(edges.begin(), it);
  // index of bin centre is one less since the first boundary offsets the whole
  // range
  // need to protect for case where value equals lowest bin boundary as that
  // will return &
  // not 1
  if (edgeIndex > 0)
    return edgeIndex - 1;
  else
    return edgeIndex;
}

//------------------------------------------------------------------------------
// public members
//------------------------------------------------------------------------------

/** Constructor
 * @param length size of the numeric axis
 */
NumericAxis::NumericAxis(const std::size_t &length)
    : Axis(), m_values(length), m_edges(length + 1) {}

/**
 * Constructor taking a set of centre point values
 * @param centres A vector of values to assign to the axis
 */
NumericAxis::NumericAxis(const std::vector<double> &centres)
    : Axis(), m_values(centres), m_edges(centres.size() + 1) {
  Kernel::VectorHelper::convertToBinBoundary(m_values, m_edges);
}

/** Virtual constructor
 *  @param parentWorkspace :: The workspace is not used in this implementation
 *  @returns A pointer to a copy of the NumericAxis on which the method is
 * called
 */
Axis *NumericAxis::clone(const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  return new NumericAxis(*this);
}

Axis *NumericAxis::clone(const std::size_t length,
                         const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  NumericAxis *newAxis = new NumericAxis(*this);
  newAxis->m_values.clear();
  newAxis->m_values.resize(length);
  newAxis->m_edges.clear();
  newAxis->m_edges.resize(length + 1);
  return newAxis;
}

/** Get the axis value at the position given
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex Needed for the subclass (RefAxis) method, but ignored
 * (and defaulted) here
 *  @return The value of the axis as a double
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
double NumericAxis::operator()(const std::size_t &index,
                               const std::size_t &verticalIndex) const {
  UNUSED_ARG(verticalIndex)
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1,
                                        "NumericAxis: Index out of range.");
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
    throw Kernel::Exception::IndexError(index, length() - 1,
                                        "NumericAxis: Index out of range.");
  }

  m_values[index] = value;
  // Recompute edges. Inefficient but we want this method to disappear in favour
  // of passing all values to the constructor
  Kernel::VectorHelper::convertToBinBoundary(m_values, m_edges);
}

/**
 * Treats values as bin centres and computes bin widths from the surrounding
 * values. The index returned is the index of the bin
 * @param value A value on the axis
 * @return The index closest to given value
 * @throws std::out_of_range if the value is out of range of the axis
 */
size_t NumericAxis::indexOfValue(const double value) const {
  return this->indexOfValue(value, m_edges);
}

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true if self and second axis are equal
 */
bool NumericAxis::operator==(const Axis &axis2) const {
  return equalWithinTolerance(axis2, 1e-15);
}

/** Check if two numeric axis are equivalent to a given tolerance
 *  @param axis2 :: Reference to the axis to compare to
 *  @param tolerance :: Tolerance to compare to
 *  @return true if self and second axis are equal
 */
bool NumericAxis::equalWithinTolerance(const Axis &axis2,
                                       const double tolerance) const {
  if (length() != axis2.length()) {
    return false;
  }
  const NumericAxis *spec2 = dynamic_cast<const NumericAxis *>(&axis2);
  if (!spec2) {
    return false;
  }
  // Check each value is within tolerance
  g_tolerance = tolerance;
  return std::equal(m_values.begin(), m_values.end(), spec2->m_values.begin(),
                    withinTolerance);
}

/** Returns a text label which shows the value at index and identifies the
 *  type of the axis.
 *  @param index :: The index of an axis value
 *  @return the label of the requested axis
 */
std::string NumericAxis::label(const std::size_t &index) const {
  std::string numberLabel = boost::str(boost::format("%.13f") % (*this)(index));

  // Remove all zeros up to the decimal place or a non zero value
  auto it = numberLabel.end() - 1;
  for (; it != numberLabel.begin(); --it) {
    if (*it == '0') {
      numberLabel.erase(it);
    } else if (*it == '.') {
      numberLabel.erase(it);
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
  return this->m_edges;
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
 * Sets both values & edges vectors to zero size
 */
NumericAxis::NumericAxis() {}

} // namespace API
} // namespace Mantid
