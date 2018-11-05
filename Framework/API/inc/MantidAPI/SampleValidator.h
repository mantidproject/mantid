// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_SAMPLEVALIDATOR_H_
#define MANTID_API_SAMPLEVALIDATOR_H_

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/**
  A validator which checks that sample has the required properties.
*/
class MANTID_API_DLL SampleValidator : public MatrixWorkspaceValidator {
public:
  /// Enumeration describing requirements
  enum Requirements { Shape = 0x1, Material = 0x2 };

  SampleValidator(const unsigned int flags = (Shape | Material));
  std::string getType() const;
  Kernel::IValidator_sptr clone() const override;
  std::string checkValidity(const MatrixWorkspace_sptr &value) const override;

private:
  unsigned int m_requires;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_SAMPLEVALIDATOR_H_ */
