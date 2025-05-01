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

/** ApplyMuonDetectorGrouping :

@date 2018-05-04
*/

class MANTID_MUON_DLL ApplyMuonDetectorGrouping final : public API::Algorithm {
public:
  /// (Empty) Constructor
  ApplyMuonDetectorGrouping() : API::Algorithm() {}
  /// Algorithm's name
  const std::string name() const override { return "ApplyMuonDetectorGrouping"; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Muon\\DataHandling"; }
  /// Algorithm's summary for identification
  const std::string summary() const override {
    return "Group several muon detector IDs together and perform an analysis "
           "(either counts or asymmetry).";
  }
  /// See also
  const std::vector<std::string> seeAlso() const override { return {"MuonProcess"}; }
  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;
  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Return the algorithm properties in a struct
  Muon::AnalysisOptions getUserInput();
  /// Clip Xmin/Xmax to the range in the input WS
  void clipXRangeToWorkspace(const API::WorkspaceGroup &ws, Muon::AnalysisOptions &options);
  /// Creates and analyses a workspace, if noRebin does not rebin.
  API::Workspace_sptr createAnalysisWorkspace(const API::Workspace_sptr &inputWS, bool noRebin,
                                              Muon::AnalysisOptions options);
  /// Sets algorithm properties according to options.
  void setMuonProcessAlgorithmProperties(API::IAlgorithm &alg, const AnalysisOptions &options) const;
  /// Set algorithm properties (input workspace, and period properties)
  /// according to the given options. For use with
  /// MuonProcess.
  void setMuonProcessPeriodProperties(API::IAlgorithm &alg, const API::Workspace_sptr &inputWS,
                                      const Muon::AnalysisOptions &options) const;

  void setMuonProcessAlgorithmOutputTypeProperty(IAlgorithm &alg, const Muon::AnalysisOptions &options) const;
  /// Set grouping properies of MuonProcess
  void setMuonProcessAlgorithmGroupingProperties(IAlgorithm &alg, const Muon::AnalysisOptions &options) const;

  void setMuonProcessAlgorithmTimeProperties(IAlgorithm &alg, const Muon::AnalysisOptions &options) const;
  /// Generate the name of the new workspace
  const std::string getNewWorkspaceName(const Muon::AnalysisOptions &options, const std::string &groupWSName);

  /// 26/06/18
  /// Give the "tmp_unNorm" workspace which is added to the ADS the correct
  /// name
  bool renameAndMoveUnNormWorkspace(const std::string &newName);
};

} // namespace Muon
} // namespace Mantid
