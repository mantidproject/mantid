#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGY_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGY_H_

#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/**
 * @brief The Correction struct
 * offset:: TOF offset in unit of TOF
 * factor:  TOF correction factor to multiply with
 */
struct Correction {
  Correction(double tOffset, double tFactor)
      : offset(tOffset), factor(tFactor) {}
  double offset;
  double factor;
};

/** TimeAtSampleStrategy : Strategy (technique dependent) for determining Time
  At Sample

  SampleT = PulseT + [TOF to sample]

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport TimeAtSampleStrategy {
public:
  virtual Correction calculate(const size_t &workspace_index) const = 0;
  virtual ~TimeAtSampleStrategy() = default;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGY_H_ */
