// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidAPI/Algorithm.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidCrystal/PeakAlgorithmHelpers.h"
#include <tuple>

namespace Mantid::Kernel {
class V3D;
}

namespace Mantid::Crystal {

/**
 * Using a set of offset vectors, either provided as separate lists or as a set
 * of vectors, predict whether
 */
class MANTID_CRYSTAL_DLL PredictFractionalPeaks : public API::Algorithm {
public:
  const std::string name() const override { return "PredictFractionalPeaks"; }
  const std::string summary() const override {
    return "The offsets can be from hkl values in a range of hkl values or "
           "from peaks in the input PeaksWorkspace";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"PredictPeaks"}; }
  const std::string category() const override { return "Crystal\\Peaks"; }
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  ModulationProperties getModulationInfo();
};

} // namespace Mantid::Crystal
