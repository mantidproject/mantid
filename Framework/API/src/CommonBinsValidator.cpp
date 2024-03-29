// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid::API {

/// Clone the current state
Kernel::IValidator_sptr CommonBinsValidator::clone() const { return std::make_shared<CommonBinsValidator>(*this); }

/** Checks that the bin boundaries of each histogram in the workspace are the
 * same
 * @param value :: The workspace to test
 * @return A message for users saying that bins are different, otherwise ""
 */
std::string CommonBinsValidator::checkValidity(const MatrixWorkspace_sptr &value) const {
  if (!value)
    return "Enter an existing workspace";
  if (value->isCommonBins())
    return "";
  else
    return "The workspace must have common bin boundaries for all histograms";
}

} // namespace Mantid::API
