// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_INCREASINGAXISVALIDATOR_H_
#define MANTID_API_INCREASINGAXISVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that the X axis of a workspace is increasing from
  left to right.
*/
class MANTID_API_DLL IncreasingAxisValidator : public MatrixWorkspaceValidator {
public:
  /// Get the type of the validator
  std::string getType() const { return "IncreasingAxis"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_INCREASINGAXISVALIDATOR_H_ */
