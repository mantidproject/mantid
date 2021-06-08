// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/TypedValidator.h"

/**
  A validator which checks that the frame of the MDWorkspace referred to
  by a WorkspaceProperty is the expected one.
*/
namespace Mantid {
namespace API {
class MANTID_API_DLL MDFrameValidator : public Kernel::TypedValidator<IMDWorkspace_sptr> {
public:
  explicit MDFrameValidator(const std::string &frameName);
  /// Gets the type of the validator
  std::string getType() const { return "mdframe"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const IMDWorkspace_sptr &workspace) const override;

  /// The name of the required frame
  const std::string m_frameID;
};

} // namespace API
} // namespace Mantid
