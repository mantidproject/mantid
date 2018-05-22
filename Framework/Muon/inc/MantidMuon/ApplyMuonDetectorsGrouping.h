#ifndef MANTID_MUON_APPLYMUONDETECTORSGROUPING_H_
#define MANTID_MUON_APPLYMUONDETECTORSGROUPING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

/** ApplyMuonDetectorsGrouping : TODO: DESCRIPTION

@date 2018-05-04

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport ApplyMuonDetectorGrouping : public Mantid::API::Algorithm {
public:
  /// (Empty) Constructor
  ApplyMuonDetectorGrouping() : Mantid::API::Algorithm() {}
  /// Virtual destructor
  ~ApplyMuonDetectorGrouping() {}
  /// Algorithm's name
  const std::string name() const override {
    return "ApplyMuonDetectorGrouping";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Grouping;Transforms\\Grouping";
  }
  /// Algorithm's summary for identification
  const std::string summary() const override {
    return "Group several muon detector IDs together and perform an analysis.";
  }
  /// Clip Xmin/Xmax to the range in the input WS
  void clipXRangeToWorkspace(MatrixWorkspace_sptr &ws);
  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;
  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;

private:
  std::string m_groupName;
  std::string m_groupedWS_name;
  std::string m_groups;
  std::string m_rebinArgs;
  std::string m_wsName;
  std::string m_summedPeriods;
  std::string m_subtractedPeriods;
  Muon::PlotType m_plotType;
  double m_Xmin;
  double m_Xmax;
  double m_loadedTimeZero;
  double m_timeZero;
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  ///  Store the input properties as private member variables
  WorkspaceGroup_sptr getUserInput(Workspace_sptr &inputWS,
                                   WorkspaceGroup_sptr &groupedWS);
  /// Creates and analyses a workspace, if isRaw does not rebin.
  Workspace_sptr createAnalysisWorkspace(Workspace_sptr &inputWS, bool isRaw);
  /// Sets algorithm properties according to options.
  void
  setProcessAlgorithmProperties(IAlgorithm_sptr alg,
                                const Muon::AnalysisOptions &options) const;
  /// Generate the name of the new workspace
  const std::string getNewWorkspaceName();
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_MUON_APPLYMUONDETECTORSGROUPING_H_ */
