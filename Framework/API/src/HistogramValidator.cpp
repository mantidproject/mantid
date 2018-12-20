#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** Constructor
 *
 *  @param mustBeHistogram :: Flag indicating whether the check is that a
 *  workspace should contain histogram data (true, default) or shouldn't
 * (false).
 */
HistogramValidator::HistogramValidator(const bool &mustBeHistogram)
    : MatrixWorkspaceValidator(), m_mustBeHistogram(mustBeHistogram) {}

/// Clone the current state
Kernel::IValidator_sptr HistogramValidator::clone() const {
  return boost::make_shared<HistogramValidator>(*this);
}

/** Checks if the workspace contains a histogram when it shouldn't and
 * vice-versa
 *  @param value :: The workspace to test
 *  @return A user level description if a problem exists or ""
 */
std::string
HistogramValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  if (m_mustBeHistogram) {
    if (value->isHistogramData())
      return "";
    else
      return "The workspace must contain histogram data";
  } else {
    if (!value->isHistogramData())
      return "";
    else
      return "The workspace must not contain histogram data";
  }
}

} // namespace API
} // namespace Mantid
