// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace API {

/** Constructor
 *
 * @param unitID :: The name of the unit that the workspace must have. If
 * left empty, the validator will simply check that the workspace is not
 * unitless.
 */
WorkspaceUnitValidator::WorkspaceUnitValidator(const std::string &unitID)
    : MatrixWorkspaceValidator(), m_unitID(unitID) {}

/**
 * Clone the current state
 */
Kernel::IValidator_sptr WorkspaceUnitValidator::clone() const {
  return std::make_shared<WorkspaceUnitValidator>(*this);
}

/** Checks that the units of the workspace data are declared match any
 * required units
 *
 * @param value :: The workspace to test
 * @return A user level description of the error or "" for no error
 */
std::string WorkspaceUnitValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  // This effectively checks for single-valued workspaces
  if (value->axes() == 0)
    return "A single valued workspace has no unit, which is required for "
           "this algorithm";

  Kernel::Unit_const_sptr unit = value->getAxis(0)->unit();
  // If m_unitID is empty it means that the workspace must have units, which
  // can be anything
  if (m_unitID.empty()) {
    return (unit && (!std::dynamic_pointer_cast<const Kernel::Units::Empty>(unit)) ? ""
                                                                                   : "The workspace must have units");
  }
  // now check if the units of the workspace is correct
  else {
    if ((!unit) || (unit->unitID().compare(m_unitID))) {
      return "The workspace must have units of " + m_unitID; //+ "; its unit is: " + unit->caption();
    } else
      return "";
  }
}

} // namespace API
} // namespace Mantid
