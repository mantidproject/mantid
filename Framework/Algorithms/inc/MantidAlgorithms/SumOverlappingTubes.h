#ifndef MANTID_ALGORITHMS_SUMOVERLAPPINGTUBES_H_
#define MANTID_ALGORITHMS_SUMOVERLAPPINGTUBES_H_

#include "MantidAlgorithms/DllConfig.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Progress.h"

#include <list>

namespace Mantid {
namespace Algorithms {

/** SumOverlappingTubes : Converts workspaces containing an instrument with PSD
  tubes into a workspace with counts as a function of height and scattering
  angle. Currently works for the D2B instrument geometry.

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
class MANTID_ALGORITHMS_DLL SumOverlappingTubes : public API::Algorithm {
public:
  const std::string name() const override { return "SumOverlappingTubes"; }
  const std::string category() const override { return "ILL\\Diffraction"; }
  const std::string summary() const override {
    return "Takes workspaces containing an instrument with PSD and tubes, and "
           "converts to a workspace with counts as a function of height and "
           "scattering angle. Detector scans with overlapping tubes are "
           "supported.";
  }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"SumSpectra"};
  }

private:
  void init() override;
  void exec() override;

  std::list<API::MatrixWorkspace_sptr> m_workspaceList;

  size_t m_numHistograms;
  size_t m_numPoints;

  double m_startScatteringAngle;
  double m_endScatteringAngle;
  double m_stepScatteringAngle;

  double m_startHeight;
  double m_endHeight;
  std::vector<double> m_heightAxis;
  std::string m_outputType;

  void getInputParameters();
  void getScatteringAngleBinning();
  void getHeightAxis(const std::string &componentName);
  std::vector<std::vector<double>>
  performBinning(API::MatrixWorkspace_sptr &outputWS);

  double distanceFromAngle(const int angleIndex, const double angle) const;

  int m_mirrorDetectors; /// holds the sign flipper for 2theta

  std::unique_ptr<API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SUMOVERLAPPINGTUBES_H_ */
