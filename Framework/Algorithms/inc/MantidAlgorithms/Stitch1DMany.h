#ifndef MANTID_ALGORITHMS_STITCH1DMANY_H_
#define MANTID_ALGORITHMS_STITCH1DMANY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

using namespace Mantid::API;

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
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Reflectometry"; }
  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Stitches histogram matrix workspaces together";
  }
  /// Validates algorithm inputs
  std::map<std::string, std::string> validateInputs() override;
  /// Validates algorithm inputs for group workspaces
  void validateGroupWorkspacesInputs();
  /// Validates inputs common to group and non-group workspaces
  std::map<std::string, std::string> validateCommonInputs();

  /// Performs the Stitch1D algorithm at a specific workspace index
  void doStitch1D(MatrixWorkspace_sptr lhsWS, MatrixWorkspace_sptr rhsWS,
                  size_t wsIndex, std::vector<double> startOverlaps,
                  std::vector<double> endOverlaps, std::vector<double> params,
                  bool scaleRhsWS, bool useManualScaleFactor,
                  double manualScaleFactor, MatrixWorkspace_sptr &outWS,
                  double &outScaleFactor);

  /// Performs the Stitch1DMany algorithm at a specific period
  void doStitch1DMany(std::vector<WorkspaceGroup_sptr> inputWSGroups,
                      size_t period, bool storeInADS,
                      std::vector<double> startOverlaps,
                      std::vector<double> endOverlaps,
                      std::vector<double> params, bool scaleRhsWS,
                      bool useManualScaleFactor, double manualScaleFactor,
                      std::string &outName,
                      std::vector<double> &outScaleFactors);

  /// For (multiperiod) workspace groups
  bool checkGroups() override;
  bool processGroups() override;

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method.
  void exec() override;

  // Data
  std::vector<std::vector<Workspace_sptr>> m_inputWSMatrix;
  std::vector<WorkspaceGroup_sptr> m_inputWSGroups;
  std::vector<double> m_startOverlaps;
  std::vector<double> m_endOverlaps;
  std::vector<double> m_params;
  std::vector<double> m_scaleFactors;
  Mantid::API::Workspace_sptr m_outputWorkspace;

  size_t m_numWSPerPeriod = 0;
  size_t m_numWSPerGroup = 0;
  double m_manualScaleFactor = 1.0;
  bool m_scaleRHSWorkspace = true;
  bool m_useManualScaleFactor = false;
  int m_scaleFactorFromPeriod = 0;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_STITCH1DMANY_H_ */
