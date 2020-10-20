// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/** EqualBinSizesValidator : Checks that all bins in a workspace are
    equally sized to within a given tolerance.
*/
class MANTID_API_DLL EqualBinSizesValidator : public MatrixWorkspaceValidator {
public:
  /// Constructor: sets properties
  EqualBinSizesValidator(const double errorLevel);
  /// Gets the type of the validator
  std::string getType() const { return "equalbinsizes"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;
  /// Error threshold
  const double m_errorLevel;
};

} // namespace API
} // namespace Mantid
