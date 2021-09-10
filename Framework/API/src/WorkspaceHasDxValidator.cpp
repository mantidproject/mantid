// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceHasDxValidator.h"

namespace Mantid {
namespace API {

/// Return a deep clone of this validator.
Kernel::IValidator_sptr WorkspaceHasDxValidator::clone() const {
  return std::make_shared<WorkspaceHasDxValidator>(*this);
}

/// Return an error string if not all histograms in ws have Dx, otherwise an
/// empty string.
std::string WorkspaceHasDxValidator::checkValidity(MatrixWorkspace_sptr const &ws) const {
  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    if (!ws->hasDx(i)) {
      return "The workspace must have Dx values set";
    }
  }
  return "";
}

} // namespace API
} // namespace Mantid
