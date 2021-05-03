// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SampleShapeValidator.h"
#include "MantidAPI/Sample.h"
#include <memory>

namespace Mantid {
namespace API {

/// @return A string identifier for the type of validator
std::string SampleShapeValidator::getType() const { return "SampleShape"; }

/// @return A copy of the validator as a new object
Kernel::IValidator_sptr SampleShapeValidator::clone() const { return std::make_shared<SampleShapeValidator>(); }

/**
 * Checks that the workspace has a valid sample shape defined
 *  @param value :: The workspace to test
 *  @return A user level description if a problem exists or ""
 */
std::string SampleShapeValidator::checkValidity(const std::shared_ptr<ExperimentInfo> &value) const {
  const auto &sampleShape = value->sample().getShape();
  if (sampleShape.hasValidShape()) {
    return "";
  } else {
    return "Invalid or no shape defined for sample";
  }
}

} // namespace API
} // namespace Mantid
