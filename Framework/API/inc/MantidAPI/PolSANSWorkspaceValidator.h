// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TypedValidator.h"

namespace Mantid {
namespace API {

/** PolSANSWorkspaceValidator : Validator for SANS polarized transmission runs
 */
class MANTID_API_DLL PolSANSWorkspaceValidator : public Kernel::TypedValidator<WorkspaceGroup_sptr> {
public:
  explicit PolSANSWorkspaceValidator(bool expectHistogramData = true, bool allowMultiPeriodData = false);
  /// Gets the type of the validator
  std::string getType() const { return "polSANS"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const WorkspaceGroup_sptr &workspace) const override;
  const std::string validateGroupItem(API::MatrixWorkspace_sptr const &workspace) const;

  const bool m_expectHistogramData;
  const bool m_allowMultiPeriodData;
};

} // namespace API
} // namespace Mantid
