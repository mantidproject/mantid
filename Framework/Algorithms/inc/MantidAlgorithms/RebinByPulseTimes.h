// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/RebinByTimeBase.h"

namespace Mantid {
namespace Algorithms {

/** RebinByPulseTimes : Rebin an input EventWorkspace according to the pulse
 times of the events.
 */
class MANTID_ALGORITHMS_DLL RebinByPulseTimes : public RebinByTimeBase {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Bins events according to pulse time. Binning parameters are "
           "specified relative to the start of the run.";
  }

  int version() const override;

  const std::string category() const override;
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override { return {"Rebin", "RebinByTimeAtSample"}; }

private:
  /// Do the algorithm specific histogramming.
  void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS, Mantid::API::MatrixWorkspace_sptr outputWS,
                       Mantid::MantidVecPtr &XValues_new, Mantid::MantidVec &OutXValues_scaled,
                       Mantid::API::Progress &prog) override;

  /// Get the minimum x across all spectra in workspace
  uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr) const override;
  /// Get the maximum x across all spectra in workspace
  uint64_t getMinX(Mantid::API::IEventWorkspace_sptr) const override;
};

} // namespace Algorithms
} // namespace Mantid
