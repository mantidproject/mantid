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

class MANTID_MUON_DLL MuonGroupingAsymmetry final : public API::Algorithm {
public:
  MuonGroupingAsymmetry() : API::Algorithm() {}

  const std::string name() const override { return "MuonGroupingAsymmetry"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Apply an estimate of the asymmetry to a particular detector "
           "grouping in Muon data.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess",
            "EstimateMuonAsymmetryFromCounts",
            "Minus",
            "Plus",
            "MuonGroupingCounts",
            "MuonPairingAsymmetry",
            "MuonPreProcess"};
  }

private:
  void init() override;
  void exec() override;

  std::map<std::string, std::string> validateInputs() override;

  WorkspaceGroup_sptr createGroupWorkspace(const WorkspaceGroup_sptr &inputWS);

  void addGroupingAsymmetrySampleLogs(const MatrixWorkspace_sptr &workspace);
};

} // namespace Muon
} // namespace Mantid
