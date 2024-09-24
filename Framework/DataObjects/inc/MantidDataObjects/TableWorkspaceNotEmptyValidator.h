// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/TableWorkspaceValidator.h"

namespace Mantid {
namespace DataObjects {

/**
  A validator which checks that the table workspace is not empty.
*/
class MANTID_DATAOBJECTS_DLL TableWorkspaceNotEmptyValidator : public TableWorkspaceValidator {
public:
  explicit TableWorkspaceNotEmptyValidator();
  /// Gets the type of the validator
  std::string getType() const { return "notempty"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const TableWorkspace_sptr &value) const override;
};

} // namespace DataObjects
} // namespace Mantid
