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

/** ApplyDetailedBalance : Convert to chi'' of an MDEvent workspace

  @date 2011-11-08
*/
class MANTID_MDALGORITHMS_DLL ApplyDetailedBalanceMD final : public API::Algorithm {
public:
  ApplyDetailedBalanceMD() : mDeltaEIndex(999) {}
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
  void applyDetailedBalance(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// Get temperature
  std::string getTemperature(const API::IMDEventWorkspace_sptr &mdws);
  /// Check input workspace dimension
  std::string checkInputMDDimension();

  /// index of the MD dimension index for DeltaE
  size_t mDeltaEIndex;

  /// map of temperature retrieved from sample logs
  std::map<uint16_t, double> mExpinfoTemperatureMean;
};

} // namespace MDAlgorithms
} // namespace Mantid
