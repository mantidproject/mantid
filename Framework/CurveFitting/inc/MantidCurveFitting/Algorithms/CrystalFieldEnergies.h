// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/DllConfig.h"
namespace Mantid {
namespace CurveFitting {

/** CrystalFieldEnergies : Calculates crystal field energies
  and wave functions for rare earth ions given the field parameters.
*/
class MANTID_CURVEFITTING_DLL CrystalFieldEnergies final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace CurveFitting
} // namespace Mantid
