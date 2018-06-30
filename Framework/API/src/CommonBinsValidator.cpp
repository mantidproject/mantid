#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/// Clone the current state
Kernel::IValidator_sptr CommonBinsValidator::clone() const {
  return boost::make_shared<CommonBinsValidator>(*this);
}

/** Checks that the bin boundaries of each histogram in the workspace are the
 * same
 * @param value :: The workspace to test
 * @return A message for users saying that bins are different, otherwise ""
 */
std::string
CommonBinsValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  if (!value)
    return "Enter an existing workspace";
  if (value->isCommonBins())
    return "";
  else
    return "The workspace must have common bin boundaries for all histograms";
}

} // namespace API
} // namespace Mantid
