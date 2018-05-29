#ifndef MANTID_MUON_APPLYMUONDETECTORSGROUPING_H_
#define MANTID_MUON_APPLYMUONDETECTORSGROUPING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

namespace Mantid {
namespace Muon {

/** ApplyMuonDetectorsGrouping :

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

class DLLExport ApplyMuonDetectorGrouping : public API::Algorithm {
public:
  /// (Empty) Constructor
  ApplyMuonDetectorGrouping() : API::Algorithm() {}
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
    return "Group several muon detector IDs together and perform an analysis "
           "(either counts or asymmetry).";
  }
  /// See also
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess"};
  }
  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;
  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  ///  Store the input properties as private member variables
  void getUserInput(const API::Workspace_sptr &inputWS,
                    Muon::AnalysisOptions &options);
  /// Clip Xmin/Xmax to the range in the input WS
  void clipXRangeToWorkspace(const API::WorkspaceGroup &ws,
                             Muon::AnalysisOptions &options);
  /// Creates and analyses a workspace, if noRebin does not rebin.
  API::Workspace_sptr
  createAnalysisWorkspace(const API::Workspace_sptr &inputWS, bool noRebin,
                          Muon::AnalysisOptions &options);
  /// Sets algorithm properties according to options.
  void setMuonProcessAlgorithmProperties(API::IAlgorithm &alg,
                                         const AnalysisOptions &options) const;
  /// Convert the input workspace into a workspace group if e.g. it has only a
  /// single period otherwise leave it alone.
  API::WorkspaceGroup_sptr
  convertInputWStoWSGroup(const API::Workspace_sptr inputWS);
  /// Set algorithm properties (input workspace, and period properties)
  /// according to the given options. For use with
  /// MuonProcess.
  void
  setMuonProcessPeriodProperties(API::IAlgorithm &alg,
                                 const API::Workspace_sptr &inputWS,
                                 const Muon::AnalysisOptions &options) const;

  void setMuonProcessAlgorithmOutputTypeProperty(
      IAlgorithm &alg, const Muon::AnalysisOptions &options) const;
  /// Set grouping properies of MuonProcess
  void setMuonProcessAlgorithmGroupingProperties(
      IAlgorithm &alg, const Muon::AnalysisOptions &options) const;

  void setMuonProcessAlgorithmTimeProperties(
      IAlgorithm &alg, const Muon::AnalysisOptions &options) const;
  /// Generate the name of the new workspace
  const std::string getNewWorkspaceName(const Muon::AnalysisOptions &options,
                                        const std::string &groupWSName);
  /// Add the grouped counts to a workspace named "MuonAanalysisGrouped" if its
  /// in the ADS
  void addCountsToMuonAnalysisGrouped(const API::Workspace_sptr &inputWS,
                                      const Muon::AnalysisOptions options);
  /// If "MuonAnalysisGrouped" in ADS and a grouped workspace return the
  /// workspace names it contains, else return "MuonAnalysisGrouped"
  std::vector<std::string> getMuonAnalysisGroupedWorkspaceNames();
  /// Delete a workspace if it is in the ADS
  void deleteWorkspaceIfExists(const std::string &name);
};

} // namespace Muon
} // namespace Mantid

#endif /* MANTID_MUON_APPLYMUONDETECTORSGROUPING_H_ */
