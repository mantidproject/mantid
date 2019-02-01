// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SPECTRAAXISVALIDATOR_H_
#define MANTID_API_SPECTRAAXISVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks whether the input workspace has the Spectra number
  in the axis.
*/
class MANTID_API_DLL SpectraAxisValidator : public MatrixWorkspaceValidator {
public:
  explicit SpectraAxisValidator(const int &axisNumber = 1);

  /// Gets the type of the validator
  std::string getType() const { return "spectraaxis"; }
  /// Clone the current validator
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;
  /// Axis number to check on, defaults to 1
  const int m_axisNumber;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SPECTRAAXISVALIDATOR_H_ */
