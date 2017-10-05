#include "MantidAPI/SingleCountValidator.h"

namespace Mantid {
namespace API {

/** Constructor
 *
 *  @param mustBeSingleCount :: Flag indicating whether the check is that a
 *  workspace should contain histogram data (true, default) or shouldn't
 * (false).
 */
SingleCountValidator::SingleCountValidator(const bool &mustBeSingleCount)
    : MatrixWorkspaceValidator(), m_mustBeSingleCount(mustBeSingleCount) {}

/// Clone the current state
Kernel::IValidator_sptr SingleCountValidator::clone() const {
  return boost::make_shared<SingleCountValidator>(*this);
}

/** Checks if the workspace contains a single counts when it should not and
 * vice-versa
 *  @param ws The workspace to validate
 *  @return A user level description if a problem exists, otherwise an empty
 * string
 */
std::string
SingleCountValidator::checkValidity(const MatrixWorkspace_sptr &ws) const {
  size_t blockSize = -1;
  try {
    blockSize = ws->blocksize();
  } catch (std::length_error &) {
    if (m_mustBeSingleCount)
      return "The workspace must contain single counts for all spectra, but it "
             "is variably sized";
  }

  if (m_mustBeSingleCount) {
    if (blockSize == 1)
      return "";
    else
      return "The workspace must contain single counts for all spectra";
  } else {
    if (blockSize != 1)
      return "";
    else
      return "The workspace must not contain single counts";
  }
}

} // namespace API
} // namespace Mantid
