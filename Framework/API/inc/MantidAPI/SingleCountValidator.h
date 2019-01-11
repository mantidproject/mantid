// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SINGLECOUNTVALIDATOR_H_
#define MANTID_API_SINGLECOUNTVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/** SingleCountValidator : This validator checks that there is only a single
  entry per spectrum, the counts, so no Time-of-Flight data. Warning: only the
  first bin of the workspace is checked, for performance reasons.
*/
class MANTID_API_DLL SingleCountValidator : public MatrixWorkspaceValidator {
public:
  explicit SingleCountValidator(const bool &mustBeSingleCounts = true);

  /// Gets the type of the validator
  std::string getType() const { return "single_count"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &ws) const override;

  /// A flag indicating whether this validator requires that the workspace be
  /// contain only single counts or not
  const bool m_mustBeSingleCount;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SINGLECOUNTVALIDATOR_H_ */
