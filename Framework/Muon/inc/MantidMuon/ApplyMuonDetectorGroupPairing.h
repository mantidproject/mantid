#ifndef MANTID_MUON_APPLYMUONDETECTORGROUPPAIRING_H_
#define MANTID_MUON_APPLYMUONDETECTORGROUPPAIRING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/System.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

namespace Mantid {
namespace Muon {

/** ApplyMuonDetectorGroupPairing :

@date 2018-06-01

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

class DLLExport ApplyMuonDetectorGroupPairing : public API::Algorithm {
public:
  /// (Empty) Constructor
  ApplyMuonDetectorGroupPairing() : API::Algorithm() {}
  /// Virtual destructor
  ~ApplyMuonDetectorGroupPairing() {}
  /// Algorithm's name
  const std::string name() const override {
    return "ApplyMuonDetectorGroupPairing";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Muon\\DataHandling";
  }
  /// Algorithm's summary for identification
  const std::string summary() const override {
    return "Perform an asymmetry analysis on two groupings of muon detectors.";
  }
  /// See also
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess", "ApplyMuonDetectorGrouping",
            "LoadAndApplyMuonDetectorGrouping"};
  }
  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;

  /// Get the names of the two workspaces in the ADS to pair manually.
  const std::string
  getGroupWorkspaceNamesManually(const std::string &groupName,
                                 const std::string &groupWSName);

  /// Get the name of the workspace to be saved.
  const std::string getPairWorkspaceName(const std::string &pairName,
                                         const std::string &groupWSName);

  /// return a workspace for a pair of detector groups, specified in options.
  API::MatrixWorkspace_sptr
  createPairWorkspaceManually(API::Workspace_sptr inputWS, const bool noRebin);

  /// Store the input properties in options.
  Muon::AnalysisOptions getUserInput();

  /// Set MuonProcess properties (input workspace and period properties).
  void
  setMuonProcessPeriodProperties(IAlgorithm &alg, API::Workspace_sptr inputWS,
                                 const Muon::AnalysisOptions &options) const;

  /// Set MuonProcess properties according to the given options.
  void
  setMuonProcessAlgorithmProperties(IAlgorithm &alg,
                                    const Muon::AnalysisOptions &options) const;

  /// Apply the asymmetry calculation to two workspaces.
  API::MatrixWorkspace_sptr
  createPairWorkspaceFromGroupWorkspaces(API::MatrixWorkspace_sptr inputWS1,
                                         API::MatrixWorkspace_sptr inputWS2,
                                         const double &alpha);

  /// Set grouping properties of MuonProcess
  void setMuonProcessAlgorithmGroupingProperties(
      IAlgorithm &alg, const Muon::AnalysisOptions &options) const;

  /// Set time properties of MuonProcess according to the given options.
  void setMuonProcessAlgorithmTimeProperties(
      IAlgorithm &alg, const Muon::AnalysisOptions &options) const;

  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Muon
} // namespace Mantid

#endif /* MANTID_MUON_APPLYMUONDETECTORGROUPPAIRING_H_ */
