// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

using Mantid::DataObjects::MDEventWorkspace;

namespace Mantid {
namespace MDAlgorithms {

class MANTID_MDALGORITHMS_DLL DgsScatteredTransmissionCorrectionMD final : public API::Algorithm {
public:
  DgsScatteredTransmissionCorrectionMD() {}
  const std::string name() const override { return "DgsScatteredTransmissionCorrectionMD"; }
  int version() const override { return 1; }
  const std::string category() const override { return "MDAlgorithms"; }
  const std::string summary() const override {
    return "Isotropic scattering transmission correction for Inelastic data";
  }

private:
  /// Value of Efixed for each run
  std::vector<float> mEfixedValues;
  /// Initialize the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Verify the input properties meets certain requirements.
  std::map<std::string, std::string> validateInputs() override;
  /**
   * @brief Verify the input workspace meets certain requirements.
   * @details verify the input workspace is of type MDEventWorkspace; meets certain dimensions requirements;
   * and has positive "Ei" metadata
   */
  std::string checkInputWorkspace();
  /// Verify the input Workspace dimensions are either QSample (or QLab) frame plus DeltaE, or just |Q| plus DeltaE
  std::string checkInputMDDimensions();
  /// Apply the transmission correction to each event
  template <typename MDE, size_t nd> void correctForTransmission(typename MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace MDAlgorithms
} // namespace Mantid
