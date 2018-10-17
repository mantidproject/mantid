// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_EQUALBINSIZESVALIDATOR_H_
#define MANTID_API_EQUALBINSIZESVALIDATOR_H_

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
  EqualBinSizesValidator(const double errorLevel,
                         const double warningLevel = -1);
  /// Gets the type of the validator
  std::string getType() const { return "equalbinsizes"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;
  /// Error threshold
  const double m_errorLevel;
  /// Warning threshold
  const double m_warningLevel;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_EQUALBINSIZESVALIDATOR_H_ */