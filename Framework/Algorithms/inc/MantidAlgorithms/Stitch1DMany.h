#ifndef MANTID_ALGORITHMS_STITCH1DMANY_H_
#define MANTID_ALGORITHMS_STITCH1DMANY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** Stitch1DMany : Stitches multiple Matrix Workspaces together into a single
 output.

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
class DLLExport Stitch1DMany : public API::Algorithm {
public:
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string name() const override { return "Stitch1DMany"; }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin", "Stitch1D"};
  }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Stitches histogram matrix workspaces together";
  }
  /// Validates algorithm inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Performs the Stitch1D algorithm at a specific workspace index
  void doStitch1D(std::vector<API::MatrixWorkspace_sptr> &toStitch,
                  const std::vector<double> &manualScaleFactors,
                  API::Workspace_sptr &outWS, std::string &outName);

  /// Performs the Stitch1DMany algorithm at a specific period
  void doStitch1DMany(const size_t period, const bool useManualScaleFactors,
                      std::string &outName,
                      std::vector<double> &outScaleFactors);

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method.
  void exec() override;

  // A 2D matrix holding workspaces obtained from each workspace list/group
  std::vector<std::vector<API::MatrixWorkspace_sptr>> m_inputWSMatrix;

  std::vector<double> m_startOverlaps;
  std::vector<double> m_endOverlaps;
  std::vector<double> m_params;
  std::vector<double> m_scaleFactors;
  std::vector<double> m_manualScaleFactors;
  API::Workspace_sptr m_outputWorkspace;

  bool m_scaleRHSWorkspace = true;
  bool m_useManualScaleFactors = false;
  size_t m_scaleFactorFromPeriod = 1;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_STITCH1DMANY_H_ */
