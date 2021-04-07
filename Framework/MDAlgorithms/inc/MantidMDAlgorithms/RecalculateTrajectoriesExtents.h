// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** RecalculateTrajectoriesExtents :
 */
class MANTID_MDALGORITHMS_DLL RecalculateTrajectoriesExtents : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CropWorkspaceForMDNorm", "MDNormSCD", "MDNormDirectSC"};
  }

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override final;
};

} // namespace MDAlgorithms
} // namespace Mantid
