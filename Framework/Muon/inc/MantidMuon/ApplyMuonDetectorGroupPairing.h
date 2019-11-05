// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MUON_APPLYMUONDETECTORGROUPPAIRING_H_
#define MANTID_MUON_APPLYMUONDETECTORGROUPPAIRING_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidMuon/DllConfig.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

namespace Mantid {
namespace Muon {

/** ApplyMuonDetectorGroupPairing :

@date 2018-06-01
*/

class MANTID_MUON_DLL ApplyMuonDetectorGroupPairing : public API::Algorithm {
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
  const std::string category() const override { return "Muon\\DataHandling"; }
  /// Algorithm's summary for identification
  const std::string summary() const override {
    return "Perform an asymmetry analysis on two groupings of muon detectors.";
  }
  /// See also
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess", "ApplyMuonDetectorGrouping"};
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

  /// Checks that the detector IDs in grouping are in the workspace
  void checkDetectorIDsInWorkspace(API::Grouping &grouping,
                                   API::Workspace_sptr workspace);

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
