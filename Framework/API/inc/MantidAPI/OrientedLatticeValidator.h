// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/TypedValidator.h"

/**
  A validator which checks that a workspace has an oriented lattice attached to
  it.
*/
namespace Mantid {
namespace API {
class MANTID_API_DLL OrientedLatticeValidator : public Kernel::TypedValidator<ExperimentInfo_sptr> {
public:
  /// Gets the type of the validator
  std::string getType() const { return "orientedlattice"; }
  /// Clone the current state
  Kernel::IValidator_sptr clone() const override;

private:
  /// Check for validity.
  std::string checkValidity(const ExperimentInfo_sptr &workspace) const override;
};

} // namespace API
} // namespace Mantid
