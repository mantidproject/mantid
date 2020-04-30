// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SingleSpectrumValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {

/** Constructor
 *
 *  @param mustBeSingleSpectrum :: Flag indicating whether the check is that a
 *  workspace should contain a single spectrum (true, default) or shouldn't
 * (false).
 */
SingleSpectrumValidator::SingleSpectrumValidator(
    const bool &mustBeSingleSpectrum)
    : MatrixWorkspaceValidator(), m_mustBeSingleSpectrum(mustBeSingleSpectrum) {
}

/// Clone the current state
Kernel::IValidator_sptr SingleSpectrumValidator::clone() const {
  return std::make_shared<SingleSpectrumValidator>(*this);
}

/** Checks if the workspace contains a single spectrum when it shouldn't and
 * vice-versa
 *  @param value :: The workspace to test
 *  @return A user level description if a problem exists or ""
 */
std::string SingleSpectrumValidator::checkValidity(
    const MatrixWorkspace_sptr &value) const {
  if (m_mustBeSingleSpectrum) {
    if (value->getNumberHistograms())
      return "";
    else
      return "The workspace must contain a single spectrum";
  } else {
    if (!value->getNumberHistograms())
      return "";
    else
      return "The workspace must contain more than one spectrum";
  }
}

} // namespace API
} // namespace Mantid
