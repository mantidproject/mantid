// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/OrientedLatticeValidator.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/IValidator.h"
#include <memory>

using Mantid::Kernel::IValidator_sptr;

namespace Mantid {
namespace API {

/**
 * Clone the current state
 */
Kernel::IValidator_sptr OrientedLatticeValidator::clone() const {
  return std::make_shared<OrientedLatticeValidator>(*this);
}

/** Checks that workspace has an oriented lattice.
 *
 * @param info :: The experiment info to check for an oriented lattice.
 * @return A user level description of the error or "" for no error
 */
std::string OrientedLatticeValidator::checkValidity(const ExperimentInfo_sptr &info) const {
  if (!info->sample().hasOrientedLattice()) {
    return "Workspace must have a sample with an orientation matrix defined.";
  } else {
    return "";
  }
}

} // namespace API
} // namespace Mantid
