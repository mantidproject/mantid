// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidDataObjects/TableWorkspaceNotEmptyValidator.h"

namespace Mantid::DataObjects {

/** Constructor
 *
 */
TableWorkspaceNotEmptyValidator::TableWorkspaceNotEmptyValidator() : TableWorkspaceValidator() {}

/**
 * Clone the current state
 */
Kernel::IValidator_sptr TableWorkspaceNotEmptyValidator::clone() const {
  return std::make_shared<TableWorkspaceNotEmptyValidator>(*this);
}

/** Checks that the workspace is not empty
 *
 * @return A user level description of the error or "" for no error
 */
std::string TableWorkspaceNotEmptyValidator::checkValidity(const TableWorkspace_sptr &value) const {
  if (value->columnCount() == 0) {
    return "The workspace must have at least 1 column";
  }
  if (value->rowCount() == 0) {
    return "The workspace must have at least 1 row";
  }

  return "";
}

} // namespace Mantid::DataObjects
