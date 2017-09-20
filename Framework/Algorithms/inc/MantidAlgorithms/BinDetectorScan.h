#ifndef MANTID_ALGORITHMS_BINDETECTORSCAN_H_
#define MANTID_ALGORITHMS_BINDETECTORSCAN_H_

#include "MantidAlgorithms/DllConfig.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** BinDetectorScan : TODO: DESCRIPTION

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL BinDetectorScan : public API::Algorithm {
public:
  const std::string name() const override { return "BinDetectorScan"; }
  const std::string summary() const override { return "TODO: fill this out."; }
  std::map<std::string, std::string> validateInputs() override;
  int version() const override { return 1; }

private:
  void init() override;
  void exec() override;

  std::list<API::MatrixWorkspace_sptr> m_workspaceList;

  size_t m_numHistograms;
  size_t m_numPoints;

  double m_startScatteringAngle;
  double m_endScatteringAngle;
  double m_stepScatteringAngle;

  std::vector<double> m_heightAxis;

  void getInputParameters();
  void getScatteringAngleBinning();
  void getHeightAxis();
  void performBinning(API::MatrixWorkspace_sptr &outputWS);

  double distanceFromAngle(const size_t thetaIndex, const double theta) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_BINDETECTORSCAN_H_ */
