#ifndef MANTID_ALGORITHMS_REBINBYPULSETIMES_H_
#define MANTID_ALGORITHMS_REBINBYPULSETIMES_H_

#include "MantidAlgorithms/RebinByTimeBase.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** RebinByPulseTimes : Rebin an input EventWorkspace according to the pulse
 times of the events.

 Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport RebinByPulseTimes : public RebinByTimeBase {
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
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin", "RebinByTimeAtSample"};
  }

private:
  /// Do the algorithm specific histogramming.
  void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS,
                       Mantid::API::MatrixWorkspace_sptr outputWS,
                       Mantid::MantidVecPtr &XValues_new,
                       Mantid::MantidVec &OutXValues_scaled,
                       Mantid::API::Progress &prog) override;

  /// Get the minimum x across all spectra in workspace
  uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr) const override;
  /// Get the maximum x across all spectra in workspace
  uint64_t getMinX(Mantid::API::IEventWorkspace_sptr) const override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBINBYPULSETIMES_H_ */
