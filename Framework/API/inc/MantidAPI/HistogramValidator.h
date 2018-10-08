// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_HISTOGRAMVALIDATOR_H_
#define MANTID_API_HISTOGRAMVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that a workspace contains histogram data (the
  default) or point data as required.
*/
class MANTID_API_DLL HistogramValidator : public MatrixWorkspaceValidator {
public:
  explicit HistogramValidator(const bool &mustBeHistogram = true);

  /// Gets the type of the validator
  std::string getType() const { return "histogram"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

  /// A flag indicating whether this validator requires that the workspace be a
  /// histogram (true) or not
  const bool m_mustBeHistogram;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_HISTOGRAMVALIDATOR_H_ */
