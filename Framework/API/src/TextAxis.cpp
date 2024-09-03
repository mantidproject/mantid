// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid::API {

/** Constructor
 * @param length the size of the text axis
 */
TextAxis::TextAxis(const std::size_t &length) : Axis() { m_values.resize(length); }

/** Virtual constructor
 *  @param parentWorkspace :: The workspace is not used in this implementation
 *  @return A pointer to a copy of the TextAxis on which the method is called
 */
Axis *TextAxis::clone(const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  return new TextAxis(*this);
}

Axis *TextAxis::clone(const std::size_t length, const MatrixWorkspace *const parentWorkspace) {
  UNUSED_ARG(parentWorkspace)
  auto newAxis = new TextAxis(*this);
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
double TextAxis::operator()(const std::size_t &index, const std::size_t &verticalIndex) const {
  UNUSED_ARG(verticalIndex)
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1, "TextAxis: Index out of range.");
  }

  return EMPTY_DBL();
}

/** Sets the axis value at a given position
 *  @param index :: The position along the axis for which to set the value
 *  @param value :: The new value
 *  @throw  IndexError If the index requested is not in the range of this axis
 */
void TextAxis::setValue(const std::size_t &index, const double &value) {
  UNUSED_ARG(index)
  UNUSED_ARG(value)
  throw std::domain_error("setValue method cannot be used on a TextAxis.");
}
/**
 * Returns the value that has been passed to it as a size_t
 */
size_t TextAxis::indexOfValue(const double value) const {
  std::vector<double> spectraNumbers;
  spectraNumbers.reserve(length());
  for (size_t i = 0; i < length(); i++) {
    spectraNumbers.emplace_back(static_cast<double>(i));
  }
  return Mantid::Kernel::VectorHelper::indexOfValueFromCenters(spectraNumbers, value);
}

/** Check if two TextAxis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true if self and other axis are equal
 */
bool TextAxis::operator==(const TextAxis &axis2) const { return compareToTextAxis(axis2); }

bool TextAxis::compareToTextAxis(const TextAxis &axis2) const {
  if (length() != axis2.length()) {
    return false;
  }
  return std::equal(m_values.begin(), m_values.end(), axis2.m_values.begin());
}

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true if self and other axis are equal
 */
bool TextAxis::operator==(const Axis &axis2) const {
  const auto *spec2 = dynamic_cast<const TextAxis *>(&axis2);
  if (!spec2) {
    return false;
  }
  return compareToTextAxis(*spec2);
}

/** Returns a text label which shows the value at index and identifies the
 *  type of the axis.
 *  @param index :: The index of an axis value
 *  @return The label
 */
std::string TextAxis::label(const std::size_t &index) const { return m_values.at(index); }

/**
 * Set the label for value at index
 * @param index :: Index
 * @param lbl :: The text label
 */
void TextAxis::setLabel(const std::size_t &index, const std::string &lbl) {
  if (index >= length()) {
    throw Kernel::Exception::IndexError(index, length() - 1, "TextAxis: Index out of range.");
  }

  m_values[index] = lbl;
}

/// returns min value defined on axis
double TextAxis::getMin() const {
  try {
    return boost::lexical_cast<double>(m_values.front());
  } catch (boost::bad_lexical_cast &) {
    return 0; // Default min
  }
}

/// returns max value defined on axis
double TextAxis::getMax() const {
  try {
    return boost::lexical_cast<double>(m_values.back());
  } catch (boost::bad_lexical_cast &) {
    return getMin() + 1; // Default max
  }
}

} // namespace Mantid::API
