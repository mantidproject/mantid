#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/Unit.h"

#include <boost/algorithm/string/join.hpp>

namespace Mantid {
namespace API {

/** Constructor
  *
  * @param unitID :: The name of the unit that the workspace must have. If
  * left empty, the validator will simply check that the workspace is not
  * unitless.
  */
WorkspaceUnitValidator::WorkspaceUnitValidator(const std::string &unitID)
    : MatrixWorkspaceValidator(), m_unitIDs() {
  if (!unitID.empty())
    m_unitIDs.push_back(unitID);
}

/** Constructor
 *
 * @param unitIDs :: The vector of names of the unit that the workspace must
 * have. If left empty, the validator will simply check that the workspace is
 * not unitless.
 */
WorkspaceUnitValidator::WorkspaceUnitValidator(
    const std::vector<std::string> &unitIDs)
    : MatrixWorkspaceValidator(), m_unitIDs(unitIDs) {}

/**
  * Clone the current state
  */
Kernel::IValidator_sptr WorkspaceUnitValidator::clone() const {
  return boost::make_shared<WorkspaceUnitValidator>(*this);
}

/** Checks that the units of the workspace data are declared match any
  * required units
  *
  * @param value :: The workspace to test
  * @return A user level description of the error or "" for no error
  */
std::string
WorkspaceUnitValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  // This effectively checks for single-valued workspaces
  if (value->axes() == 0)
    return "A single valued workspace has no unit, which is required for "
           "this algorithm";

  Kernel::Unit_const_sptr unit = value->getAxis(0)->unit();
  // If m_unitID is empty it means that the workspace must have units, which
  // can be anything
  if (m_unitIDs.empty()) {
    return (
        unit && (!boost::dynamic_pointer_cast<const Kernel::Units::Empty>(unit))
            ? ""
            : "The workspace must have units");
  }
  // now check if the units of the workspace is correct
  else {
    const auto matchesUnitID =
        [&unit](const std::string &unitID) { return unit->unitID() == unitID; };

    if (!unit ||
        !std::any_of(m_unitIDs.cbegin(), m_unitIDs.cend(), matchesUnitID)) {
      return buildErrorMessage();
    }

    return "";
  }
}

/** Build a user friendly error message.
 *
 * This will format the message differently depending on the number of unitIDs
 * checked by this validator.
 *
 * @return a user friendly error message.
 */
std::string WorkspaceUnitValidator::buildErrorMessage() const {
  if (m_unitIDs.size() == 1) {
    return "The workspace must have units of " + m_unitIDs[0];
  } else {
    return "The workspace must have one of the following units: " +
           boost::algorithm::join(m_unitIDs, ", ");
  }
}

} // namespace API
} // namespace Mantid
