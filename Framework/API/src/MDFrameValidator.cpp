#include "MantidAPI/MDFrameValidator.h"
#include "MantidKernel/IValidator.h"
#include <boost/make_shared.hpp>

using Mantid::Kernel::IValidator_sptr;

namespace Mantid {
namespace API {

/** Constructor
 *
 * @param frameName :: The name of the frame that the workspace must have.
 */
MDFrameValidator::MDFrameValidator(const std::string &frameName)
    : m_frameID{frameName} {}

/**
 * Clone the current state
 */
Kernel::IValidator_sptr MDFrameValidator::clone() const {
  return boost::make_shared<MDFrameValidator>(*this);
}

/** Checks that the frame of the MDWorkspace matches the expected frame.
 *
 * @param workspace :: The workspace to test
 * @return A user level description of the error or "" for no error
 */
std::string
MDFrameValidator::checkValidity(const IMDWorkspace_sptr &workspace) const {

  for (size_t index = 0; index < workspace->getNumDims(); ++index) {
    const auto &frame = workspace->getDimension(index)->getMDFrame();
    if (frame.name() != m_frameID)
      return "MDWorkspace must be in the " + m_frameID + " frame.";
  }

  return "";
}

} // namespace API
} // namespace Mantid
