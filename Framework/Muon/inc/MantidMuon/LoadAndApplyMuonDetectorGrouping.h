// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidMuon/DllConfig.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

namespace Mantid {
namespace Muon {

/** LoadAndApplyMuonDetectorGrouping :

@date 2018-05-31
*/

class MANTID_MUON_DLL LoadAndApplyMuonDetectorGrouping final : public API::Algorithm {
public:
  /// (Empty) Constructor
  LoadAndApplyMuonDetectorGrouping() : API::Algorithm() {}
  /// Algorithm's name
  const std::string name() const override { return "LoadAndApplyMuonDetectorGrouping"; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Muon\\DataHandling"; }
  /// Algorithm's summary for identification
  const std::string summary() const override {
    return "Load a file containing grouping/pairing infromation (XML format) "
           "and apply the grouping and pairing analysis to the input "
           "workspace.";
  }
  /// See also
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess", "ApplyMuonDetectorGrouping", "ApplyMuonDetectorGroupPairing"};
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

  API::Grouping loadGroupsAndPairs();

  /// Add all the supplied groups to the ADS, inside wsGrouped, by
  /// executing the ApplyMuonDetectorGrouping algorithm
  void addGroupingToADS(const Mantid::Muon::AnalysisOptions &options, const Mantid::API::Workspace_sptr &ws,
                        const Mantid::API::WorkspaceGroup_sptr &wsGrouped);

  /// Add all the supplied pairs to the ADS, inside wsGrouped, by
  /// executing the ApplyMuonDetectorGroupPairing algorithm
  void addPairingToADS(const Mantid::Muon::AnalysisOptions &options, const Mantid::API::Workspace_sptr &ws,
                       const Mantid::API::WorkspaceGroup_sptr &wsGrouped);

  void addGroupingInformationToADS(const Mantid::API::Grouping &grouping);

  /// Sets some default options for grouping algorithm.
  Mantid::Muon::AnalysisOptions setDefaultOptions();

  /// If no workspace group supplied, adds one with the correct name
  Mantid::API::WorkspaceGroup_sptr addGroupedWSWithDefaultName(Mantid::API::Workspace_sptr inputWS);

  /// Throw an error if the detector IDs in grouping are not in workspace
  void checkDetectorIDsInWorkspace(const Mantid::API::Grouping &grouping, const Mantid::API::Workspace_sptr &workspace);

  /// Check if the group/pair names are valid, and if all the groups which
  /// are paired are also included as groups.
  void CheckValidGroupsAndPairs(const Mantid::API::Grouping &grouping);

  void getTimeLimitsFromInputWorkspace(const Mantid::API::Workspace_sptr &inputWS,
                                       Mantid::Muon::AnalysisOptions &options);

  void getTimeLimitsFromInputs(AnalysisOptions &options);
};

} // namespace Muon
} // namespace Mantid
