// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_RAWCOUNTVALIDATOR_H_
#define MANTID_API_RAWCOUNTVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/** A validator which checks that a workspace contains raw counts in its bins.
 */
class MANTID_API_DLL RawCountValidator : public MatrixWorkspaceValidator {
public:
  explicit RawCountValidator(const bool &mustNotBeDistribution = true);

  /// Gets the type of the validator
  std::string getType() const { return "rawcount"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

  /// A flag indicating whether this validator requires that the workspace must
  /// be a distribution (false) or not (true, the default)
  const bool m_mustNotBeDistribution;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_RAWCOUNTVALIDATOR_H_ */
