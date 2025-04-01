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

using namespace Mantid::API;

namespace Mantid {
namespace Muon {

class MANTID_MUON_DLL MuonPairingAsymmetry final : public API::Algorithm {
public:
  MuonPairingAsymmetry() : API::Algorithm() {}

  const std::string name() const override { return "MuonPairingAsymmetry"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Apply a pairing asymmetry calculation between two detector groups "
           "from Muon data.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess", "MuonPreProcess", "AsymmetryCalc",         "AppendSpectra",
            "Plus",        "Minus",          "MuonGroupingAsymmetry", "MuonGroupingCounts"};
  }

private:
  void init() override;
  void exec() override;
  bool checkGroups() override;
  // Validation split across several functions due to size
  std::map<std::string, std::string> validateInputs() override;
  void validateManualGroups(std::map<std::string, std::string> &errors);
  void validateGroupsWorkspaces(std::map<std::string, std::string> &errors);
  void validatePeriods(const WorkspaceGroup_sptr &inputWS, std::map<std::string, std::string> &errors);

  WorkspaceGroup_sptr createGroupWorkspace(const WorkspaceGroup_sptr &inputWS);
  MatrixWorkspace_sptr appendSpectra(const MatrixWorkspace_sptr &inputWS1, const MatrixWorkspace_sptr &inputWS2);

  /// Perform an asymmetry calculation
  MatrixWorkspace_sptr pairAsymmetryCalc(const MatrixWorkspace_sptr &inputWS, const double &alpha);
  MatrixWorkspace_sptr calcPairAsymmetryWithSummedAndSubtractedPeriods(const std::vector<int> &summedPeriods,
                                                                       const std::vector<int> &subtractedPeriods,
                                                                       const WorkspaceGroup_sptr &groupedPeriods,
                                                                       const double &alpha);

  /// Execute the algorithm if "SpecifyGroupsManually" is checked
  MatrixWorkspace_sptr execSpecifyGroupsManually();

  MatrixWorkspace_sptr execGroupWorkspaceInput();

  void setPairAsymmetrySampleLogs(const MatrixWorkspace_sptr &workspace);
};

} // namespace Muon
} // namespace Mantid
