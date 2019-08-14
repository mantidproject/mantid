// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** Constructor
 *  @param parentWorkspace :: A pointer to the workspace that holds this axis
 */
// NumericAxis is set to length 0 since we do not need its internal storage. We
// override public functions of NumericAxis that would access it.
RefAxis::RefAxis(const MatrixWorkspace *const parentWorkspace)
    : NumericAxis(0), m_parentWS(parentWorkspace) {}

/** Private, specialised copy constructor. Needed because it's necessary to pass
 * in
 *  a pointer to the parent of the new workspace, rather than having the copy
 * point
 *  to the parent of the copied axis.
 *  @param right :: The axis to copy
 *  @param parentWorkspace :: A pointer to the parent workspace of the new axis
 */
RefAxis::RefAxis(const RefAxis &right,
                 const MatrixWorkspace *const parentWorkspace)
    : NumericAxis(right), m_parentWS(parentWorkspace) {}

/** Virtual constructor
 *  @param parentWorkspace :: A pointer to the workspace that will hold the new
 * axis
 *  @return A pointer to a copy of the Axis on which the method is called
 */
Axis *RefAxis::clone(const MatrixWorkspace *const parentWorkspace) {
  return new RefAxis(*this, parentWorkspace);
}

Axis *RefAxis::clone(const std::size_t length,
                     const MatrixWorkspace *const parentWorkspace) {
  static_cast<void>(length);
  return clone(parentWorkspace);
}

std::size_t RefAxis::length() const { return m_parentWS->x(0).size(); }

/** Get the axis value at the position given. In this case, the values are held
 * in the
 *  X vectors of the workspace itself.
 *  @param  index The position along the axis for which the value is required
 *  @param  verticalIndex The position along the orthogonal axis
 *  @return The value of the axis as a double
 *  @throw  IndexError If 'index' is not in the range of this axis
 *  @throw  std::range_error If 'verticalIndex' is not in the range of the
 * parent workspace
 */
double RefAxis::operator()(const std::size_t &index,
                           const std::size_t &verticalIndex) const {
  const auto &x = m_parentWS->x(verticalIndex);
  if (index >= x.size()) {
    throw Kernel::Exception::IndexError(index, x.size() - 1,
                                        "Axis: Index out of range.");
  }
  return x[index];
}

/** Method not available for RefAxis. Will always throw.
 * @param index location for setting
 * @param value the value to set
 */
void RefAxis::setValue(const std::size_t &index, const double &value) {
  UNUSED_ARG(index)
  UNUSED_ARG(value)
  throw std::domain_error("This method cannot be used on a RefAxis.");
}

/** Check if two axis defined as spectra or numeric axis are equivalent
 *  @param axis2 :: Reference to the axis to compare to
 *  @return true is self and second axis are equal
 */
bool RefAxis::operator==(const Axis &axis2) const {
  if (length() != axis2.length()) {
    return false;
  }
  const auto *ra2 = dynamic_cast<const RefAxis *>(&axis2);
  return ra2 != nullptr;
}

/** Check if two numeric axis are equivalent to a given tolerance
 *  @param axis2 :: Reference to the axis to compare to
 *  @param tolerance :: Tolerance to compare to
 *  @return true if self and second axis are equal
 */
bool RefAxis::equalWithinTolerance(const Axis &axis2,
                                   const double tolerance) const {
  UNUSED_ARG(tolerance);
  return this->operator==(axis2);
}

size_t RefAxis::indexOfValue(const double value) const {
  UNUSED_ARG(value)
  throw std::runtime_error("Calling indexOfValue() on RefAxis is forbidden.");
}

std::vector<double> RefAxis::createBinBoundaries() const {
  throw std::runtime_error(
      "Calling createBinBoundaries() on RefAxis is forbidden.");
}

const std::vector<double> &RefAxis::getValues() const {
  throw std::runtime_error("Calling getValues() on RefAxis is forbidded.");
}

double RefAxis::getMin() const {
  throw std::runtime_error("RefAxis cannot determine minimum value. Use readX "
                           "on the workspace instead");
}
double RefAxis::getMax() const {
  throw std::runtime_error("RefAxis cannot determine maximum value. Use readX "
                           "on the workspace instead");
}

} // namespace API
} // namespace Mantid
