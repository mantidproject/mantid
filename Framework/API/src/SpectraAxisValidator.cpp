// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid::API {

/** Class constructor with parameter.
 * @param axisNumber :: set the axis number to validate
 */
SpectraAxisValidator::SpectraAxisValidator(const int &axisNumber) : m_axisNumber(axisNumber) {}

/// Clone the current validator
Kernel::IValidator_sptr SpectraAxisValidator::clone() const { return std::make_shared<SpectraAxisValidator>(*this); }

/** Checks that the axis stated
 *  @param value :: The workspace to test
 *  @return A message for users with negative results, otherwise ""
 */
std::string SpectraAxisValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  Mantid::API::Axis const *axis;
  try {
    axis = value->getAxis(m_axisNumber);
  } catch (Kernel::Exception::IndexError &) {
    return "No axis at index " + std::to_string(m_axisNumber) + " available in the workspace";
  }

  if (axis->isSpectra())
    return "";
  else
    return "A workspace with axis being Spectra Number is required here.";
}

} // namespace Mantid::API
