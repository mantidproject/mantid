// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REBINBYTIMEATSAMPLE_H_
#define MANTID_ALGORITHMS_REBINBYTIMEATSAMPLE_H_

#include "MantidAlgorithms/RebinByTimeBase.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Algorithms {

/** RebinByTimeAtSample : Rebins an event workspace to a histogram workspace
 with time at sample along the x-axis.
 */
class DLLExport RebinByTimeAtSample : public RebinByTimeBase {
public:
  const std::string name() const override;
  int version() const override;

  const std::string category() const override;
  const std::string summary() const override;
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin", "RebinByPulseTimes"};
  }

private:
  void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS,
                       Mantid::API::MatrixWorkspace_sptr outputWS,
                       Mantid::MantidVecPtr &XValues_new,
                       Mantid::MantidVec &OutXValues_scaled,
                       Mantid::API::Progress &prog) override;

  /// Get the minimum x across all spectra in workspace
  uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr ws) const override;
  /// Get the maximum x across all spectra in workspace
  uint64_t getMinX(Mantid::API::IEventWorkspace_sptr ws) const override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBINBYTIMEATSAMPLE_H_ */
