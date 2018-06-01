#ifndef MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTION_H_
#define MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IComponent.h"

namespace Mantid {
namespace Algorithms {

/** VesuvioL1ThetaResolution

  Calculates the resolution function for L1 and Theta.

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
class DLLExport VesuvioL1ThetaResolution : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void loadInstrument();

  void calculateDetector(const Mantid::Geometry::IDetector &detector,
                         std::function<double()> &flatRandomVariateGen,
                         std::vector<double> &l1Values,
                         std::vector<double> &thetaValues);
  Mantid::API::MatrixWorkspace_sptr
  processDistribution(Mantid::API::MatrixWorkspace_sptr ws,
                      const double binWidth);

  Mantid::API::MatrixWorkspace_sptr m_instWorkspace;
  Mantid::Geometry::IComponent_const_sptr m_sample;
  Mantid::API::MatrixWorkspace_sptr m_outputWorkspace;
  Mantid::API::MatrixWorkspace_sptr m_l1DistributionWs;
  Mantid::API::MatrixWorkspace_sptr m_thetaDistributionWs;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_VESUVIOL1THETARESOLUTION_H_ */
