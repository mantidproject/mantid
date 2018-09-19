#ifndef MANTID_ALGORITHMS_REBINBYTIMEBASE_H_
#define MANTID_ALGORITHMS_REBINBYTIMEBASE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {

/** RebinByTimeBase : Algorithm base class for algorithms performing rebinning
 by an absolute time axis.

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport RebinByTimeBase : public API::Algorithm {
public:
private:
  /// Initialization method
  void init() override;
  /// execute.
  void exec() override;
  /// Do the algorithm specific histogramming.
  virtual void doHistogramming(Mantid::API::IEventWorkspace_sptr inWS,
                               Mantid::API::MatrixWorkspace_sptr outputWS,
                               Mantid::MantidVecPtr &XValues_new,
                               Mantid::MantidVec &OutXValues_scaled,
                               Mantid::API::Progress &prog) = 0;

  /// Get the minimum x across all spectra in workspace
  virtual uint64_t getMaxX(Mantid::API::IEventWorkspace_sptr ws) const = 0;
  /// Get the maximum x across all spectra in workspace
  virtual uint64_t getMinX(Mantid::API::IEventWorkspace_sptr ws) const = 0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBINBYTIMEBASE_H_ */
