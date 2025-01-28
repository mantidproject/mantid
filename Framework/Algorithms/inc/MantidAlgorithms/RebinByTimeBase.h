// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {

/** RebinByTimeBase : Algorithm base class for algorithms performing rebinning
 by an absolute time axis.
 */
class MANTID_ALGORITHMS_DLL RebinByTimeBase : public API::Algorithm {
protected:
  ~RebinByTimeBase() override = default;

private:
  /// Initialization method
  void init() override;
  /// execute.
  void exec() override;
  /// Do the algorithm specific histogramming.
  virtual void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS, Mantid::API::MatrixWorkspace_sptr outputWS,
                               Mantid::MantidVecPtr &XValues_new, Mantid::MantidVec &OutXValues_scaled,
                               Mantid::API::Progress &prog) = 0;

  /// Get the minimum x across all spectra in workspace
  virtual uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr ws) const = 0;
  /// Get the maximum x across all spectra in workspace
  virtual uint64_t getMinX(Mantid::API::IEventWorkspace_sptr ws) const = 0;
};

} // namespace Algorithms
} // namespace Mantid
