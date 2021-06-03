// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {

/** PolarizationAngleCorrection : Perform the And boolean operation on two MDHistoWorkspaces

  @date 2011-11-08
*/
class MANTID_MDALGORITHMS_DLL PolarizationAngleCorrectionMD : public API::Algorithm {
public:
  PolarizationAngleCorrectionMD() : mIsQSample(false) {}
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  /// Initialize the proeprties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Apply detailed balance to each MDEvent
  template <typename MDE, size_t nd>
  void applyPolarizationAngleCorrection(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Check input workspace dimension
  std::string checkInputMDDimension();

  /// Get Ei
  std::string getEi(API::IMDEventWorkspace_sptr mdws);

  /// Sample Ei
  std::map<uint16_t, double> mEiMap;

  /// coordinate system
  bool mIsQSample;
};

} // namespace MDAlgorithms
} // namespace Mantid
