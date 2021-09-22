// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that the unit of the workspace referred to
  by a WorkspaceProperty is the expected one.
*/
class MANTID_API_DLL WorkspaceUnitValidator : public MatrixWorkspaceValidator {
public:
  explicit WorkspaceUnitValidator(std::string unitID = "");
  /// Gets the type of the validator
  std::string getType() const { return "workspaceunit"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

  /// The name of the required unit
  const std::string m_unitID;
};

} // namespace API
} // namespace Mantid
