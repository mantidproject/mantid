#include "MantidAPI/NumericAxisValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace API {

/** Class constructor with parameter.
 * @param axisNumber :: set the axis number to validate
 */
NumericAxisValidator::NumericAxisValidator(const int &axisNumber)
    : m_axisNumber(axisNumber) {}

/// Clone the current state
Kernel::IValidator_sptr NumericAxisValidator::clone() const {
  return boost::make_shared<NumericAxisValidator>(*this);
}

/** Checks that the axis stated
 *  @param value :: The workspace to test
 *  @return A message for users with negative results, otherwise ""
 */
std::string
NumericAxisValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  Mantid::API::Axis *axis;
  try {
    axis = value->getAxis(m_axisNumber);
  } catch (Kernel::Exception::IndexError &) {
    return "No axis at index " + std::to_string(m_axisNumber) +
           " available in the workspace";
  }

  if (axis->isNumeric())
    return "";
  else
    return "A workspace with axis being a Numeric Axis is required here.";
}

} // namespace API
} // namespace Mantid
