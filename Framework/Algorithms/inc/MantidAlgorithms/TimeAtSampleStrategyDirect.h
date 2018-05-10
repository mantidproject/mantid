#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECT_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECT_H_

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class MatrixWorkspace;
}

namespace Algorithms {
/** TimeAtSampleStrategyDirect : Determine the Time at Sample corrections for a
  Direct Geometry instrument

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
class DLLExport TimeAtSampleStrategyDirect : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyDirect(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws, double ei);
  Correction calculate(const size_t &workspace_index) const override;

private:
  /// Cached L1, Ei dependent const shift
  double m_constShift;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYDIRECT_H_ */
