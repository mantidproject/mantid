// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_COMMONBINSVALIDATOR_H_
#define MANTID_API_COMMONBINSVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which provides a <I>TENTATIVE</I> check that a workspace
  contains common bins in each spectrum.
  For efficiency reasons, it only checks that the first and last spectra have
  common bins, so it is important to carry out a full check within the
  algorithm itself.
*/
class MANTID_API_DLL CommonBinsValidator : public MatrixWorkspaceValidator {
public:
  /// Gets the type of the validator
  std::string getType() const { return "commonbins"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_COMMONBINSVALIDATOR_H_ */