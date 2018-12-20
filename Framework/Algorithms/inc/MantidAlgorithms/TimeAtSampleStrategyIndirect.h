#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECT_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECT_H_

#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class MatrixWorkspace;
class SpectrumInfo;
} // namespace API

namespace Algorithms {

/** TimeAtSampleStrategyIndirect : Determine Time At Sample for an indirect
  instrument setup.

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
class DLLExport TimeAtSampleStrategyIndirect : public TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyIndirect(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws);
  Correction calculate(const size_t &workspace_index) const override;

private:
  /// Workspace to operate on
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_ws;
  const API::SpectrumInfo &m_spectrumInfo;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYINDIRECT_H_ */
