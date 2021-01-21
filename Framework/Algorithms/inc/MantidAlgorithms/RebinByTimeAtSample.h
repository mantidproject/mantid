// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/RebinByTimeBase.h"

namespace Mantid {

namespace Algorithms {

/** RebinByTimeAtSample : Rebins an event workspace to a histogram workspace
 with time at sample along the x-axis.
 */
class MANTID_ALGORITHMS_DLL RebinByTimeAtSample : public RebinByTimeBase {
public:
  const std::string name() const override;
  int version() const override;

  const std::string category() const override;
  const std::string summary() const override;
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override { return {"Rebin", "RebinByPulseTimes"}; }

private:
  void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS, Mantid::API::MatrixWorkspace_sptr outputWS,
                       Mantid::MantidVecPtr &XValues_new, Mantid::MantidVec &OutXValues_scaled,
                       Mantid::API::Progress &prog) override;

  /// Get the minimum x across all spectra in workspace
  uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr ws) const override;
  /// Get the maximum x across all spectra in workspace
  uint64_t getMinX(Mantid::API::IEventWorkspace_sptr ws) const override;
};

} // namespace Algorithms
} // namespace Mantid
