#ifndef MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTIC_H_
#define MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTIC_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidAlgorithms/TimeAtSampleStrategy.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {

namespace API {
class MatrixWorkspace;
class SpectrumInfo;
}
namespace Algorithms {

/** TimeAtSampleStrategyElastic : Time at sample stragegy for elastic scattering

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
class DLLExport TimeAtSampleStrategyElastic
    : public Mantid::Algorithms::TimeAtSampleStrategy {
public:
  TimeAtSampleStrategyElastic(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> ws);
  Correction calculate(const size_t &workspace_index) const override;

private:
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_ws;
  const API::SpectrumInfo &m_spectrumInfo;
  const Kernel::V3D m_beamDir;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_TIMEATSAMPLESTRATEGYELASTIC_H_ */
