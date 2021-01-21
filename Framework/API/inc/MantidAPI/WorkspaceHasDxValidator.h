// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/MatrixWorkspaceValidator.h"

namespace Mantid {
namespace API {

/** WorkspaceHasDxValidator : A validator which checks that all histograms in a
 * workspace have Dx values.
 */
class MANTID_API_DLL WorkspaceHasDxValidator final : public MatrixWorkspaceValidator {
public:
  Kernel::IValidator_sptr clone() const override;

private:
  std::string checkValidity(MatrixWorkspace_sptr const &ws) const override;
};

} // namespace API
} // namespace Mantid
