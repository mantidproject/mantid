// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
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

/** SpectralMomentMD : Multiply MD events by DeltaE^n

  @date 2025-11-1
 */
class MANTID_MDALGORITHMS_DLL SpectralMomentMD final : public API::Algorithm {
public:
  SpectralMomentMD() : mDeltaEIndex(999) {}
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Apply weight to each MDEvent
  template <typename MDE, size_t nd>
  void applyScaling(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);

  /// index of the MD dimension index for DeltaE
  size_t mDeltaEIndex;

  /// exponent
  int mExponent;
};

} // namespace MDAlgorithms
} // namespace Mantid
