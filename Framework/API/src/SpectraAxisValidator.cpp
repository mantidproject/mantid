#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** Class constructor with parameter.
  * @param axisNumber :: set the axis number to validate
  */
SpectraAxisValidator::SpectraAxisValidator(const int &axisNumber)
    : m_axisNumber(axisNumber) {}

/// Clone the current validator
Kernel::IValidator_sptr SpectraAxisValidator::clone() const {
  return boost::make_shared<SpectraAxisValidator>(*this);
}

/** Checks that the axis stated
*  @param value :: The workspace to test
*  @return A message for users with negative results, otherwise ""
*/
std::string
SpectraAxisValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  Mantid::API::Axis *axis = value->getAxis(m_axisNumber);
  if (axis->isSpectra())
    return "";
  else
    return "A workspace with axis being Spectra Number is required here.";
}

} // namespace API
} // namespace Mantid
