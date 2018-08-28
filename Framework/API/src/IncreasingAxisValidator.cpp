#include "MantidAPI/IncreasingAxisValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace API {

/// Clone the current state
Kernel::IValidator_sptr IncreasingAxisValidator::clone() const {
  return boost::make_shared<IncreasingAxisValidator>(*this);
}

/**
 * Checks that X axis is in the right direction.
 *
 * @param value The workspace to check
 * @return "" if is valid, otherwise a user level description of a problem
 */
std::string IncreasingAxisValidator::checkValidity(
    const MatrixWorkspace_sptr &value) const {
  const Axis *xAxis{nullptr};
  try {
    // 0 for X axis
    xAxis = value->getAxis(0);
  } catch (Kernel::Exception::IndexError &) {
    return "No X axis available in the workspace";
  }

  // Left-most axis value should be less than the right-most, if ws has
  // more than one X axis value
  if (xAxis->length() > 1 &&
      xAxis->getValue(0) >= xAxis->getValue(xAxis->length() - 1))
    return "X axis of the workspace should be increasing from left to "
           "right";
  else
    return "";
}

} // namespace API
} // namespace Mantid
