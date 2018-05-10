#include "MantidAPI/RawCountValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** Constructor
 * @param mustNotBeDistribution :: Flag indicating whether the check is that
 * a workspace should not be a distribution (true, default) or should be
 * (false).
 */
RawCountValidator::RawCountValidator(const bool &mustNotBeDistribution)
    : m_mustNotBeDistribution(mustNotBeDistribution) {}

/// Clone the current state
Kernel::IValidator_sptr RawCountValidator::clone() const {
  return boost::make_shared<RawCountValidator>(*this);
}

/** Checks if the workspace must be a distribution but isn't and vice-versa
 *  @param value :: The workspace to test
 *  @return A user level description of any problem that exists or "" no
 * problem
 */
std::string
RawCountValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  if (m_mustNotBeDistribution) {
    if (!value->isDistribution())
      return "";
    else
      return "A workspace containing numbers of counts is required here";
  } else {
    if (value->isDistribution())
      return "";
    else
      return "A workspace of numbers of counts is not allowed here";
  }
}

} // namespace API
} // namespace Mantid
