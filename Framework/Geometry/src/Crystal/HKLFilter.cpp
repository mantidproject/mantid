// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/HKLFilter.h"
#include <boost/make_shared.hpp>
#include <stdexcept>

namespace Mantid {
namespace Geometry {

/**
 * Returns a function object that wraps HKLFilter::isAllowed
 *
 * This method uses std::bind to create a function object that
 * represents the HKLFilter::isAllowed() method. This way it's
 * possible to pass the function to STL-algorithms easily (see
 * class documentation).
 *
 * @return Function object with filter function for V3D.s
 */
std::function<bool(const Kernel::V3D &)> HKLFilter::fn() const noexcept {
  return std::bind(&HKLFilter::isAllowed, this, std::placeholders::_1);
}

/// Stores the supplied filter, throws exception if filter is null.
HKLFilterUnaryLogicOperation::HKLFilterUnaryLogicOperation(
    const HKLFilter_const_sptr &filter)
    : m_operand(filter) {
  if (!m_operand) {
    throw std::runtime_error(
        "Cannot create HKLFilterUnaryLogicOperation from null operand.");
  }
}

/// Returns a description of the HKLFilterNot.
std::string HKLFilterNot::getDescription() const noexcept {
  return "!" + m_operand->getDescription();
}

/// Returns true if the wrapped filter returns false and false otherwise.
bool HKLFilterNot::isAllowed(const Kernel::V3D &hkl) const noexcept {
  return !(m_operand->isAllowed(hkl));
}

/// Stores the left-hand and right-hand side operators, throws exception if
/// either is null.
HKLFilterBinaryLogicOperation::HKLFilterBinaryLogicOperation(
    const HKLFilter_const_sptr &lhs, const HKLFilter_const_sptr &rhs)
    : m_lhs(lhs), m_rhs(rhs) {
  if (!m_lhs || !m_rhs) {
    throw std::runtime_error("Cannot construct HKLFilterBinaryLogicOperation "
                             "with one or more null-operands.");
  }
}

/// Returns a description of the HKLFilterAnd.
std::string HKLFilterAnd::getDescription() const noexcept {
  return "(" + m_lhs->getDescription() + " & " + m_rhs->getDescription() + ")";
}

/// Returns true if both wrapped filters return true.
bool HKLFilterAnd::isAllowed(const Kernel::V3D &hkl) const noexcept {
  return m_lhs->isAllowed(hkl) && m_rhs->isAllowed(hkl);
}

/// Returns a description of the HKLFilterOr.
std::string HKLFilterOr::getDescription() const noexcept {
  return "(" + m_lhs->getDescription() + " | " + m_rhs->getDescription() + ")";
}

/// Returns true if either of the wrapped filters returns true.
bool HKLFilterOr::isAllowed(const Kernel::V3D &hkl) const noexcept {
  return m_lhs->isAllowed(hkl) || m_rhs->isAllowed(hkl);
}

/**
 * Constructs an HKLFilterNot from the operand
 *
 * This function makes it easy to construct HKLFilterNot by using the
 * operator ~, which inverts the wrapped filter.
 *
 * @param filter :: HKLFilter to invert.
 * @return HKLFilterNot with the wrapped filter.
 */
const HKLFilter_const_sptr operator~(const HKLFilter_const_sptr &filter) {
  return boost::make_shared<const HKLFilterNot>(filter);
}

/**
 * Constructs an HKLFilterAnd from the operands
 *
 * This function makes it easy to construct HKLFilterAnd by using the
 * operator & directly on HKLFilter_const_sptr.
 *
 * @param lhs :: Left-hand side HKLFilter operand.
 * @param rhs :: Right-hand side HKLFilter operand.
 * @return HKLFilterAnd with two wrapped filters.
 */
const HKLFilter_const_sptr operator&(const HKLFilter_const_sptr &lhs,
                                     const HKLFilter_const_sptr &rhs) {
  return boost::make_shared<const HKLFilterAnd>(lhs, rhs);
}

/**
 * Constructs an HKLFilterOr from the operands
 *
 * This function makes it easy to construct HKLFilterOr by using the
 * operator | directly on HKLFilter_const_sptr.
 *
 * @param lhs :: Left-hand side HKLFilter operand.
 * @param rhs :: Right-hand side HKLFilter operand.
 * @return HKLFilterOr with two wrapped filters.
 */
const HKLFilter_const_sptr operator|(const HKLFilter_const_sptr &lhs,
                                     const HKLFilter_const_sptr &rhs) {
  return boost::make_shared<HKLFilterOr>(lhs, rhs);
}

} // namespace Geometry
} // namespace Mantid
