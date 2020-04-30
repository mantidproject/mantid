// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that a workspace contains histogram data (the
  default) or point data as required.
*/
class MANTID_API_DLL SingleSpectrumValidator : public MatrixWorkspaceValidator {
public:
  explicit SingleSpectrumValidator(const bool &mustBeSingleSpectrum = true);

  /// Gets the type of the validator
  std::string getType() const { return "single spectrum"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

  /// A flag indicating whether this validator requires that the workspace have
  /// only a single spectrum (true) or not
  const bool m_mustBeSingleSpectrum;
};

} // namespace API
} // namespace Mantid
