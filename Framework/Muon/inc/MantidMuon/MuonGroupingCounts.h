// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidMuon/DllConfig.h"

using namespace Mantid::API;

namespace Mantid {
namespace Muon {

class MANTID_MUON_DLL MuonGroupingCounts final : public API::Algorithm {
public:
  MuonGroupingCounts() : API::Algorithm() {}

  const std::string name() const override { return "MuonGroupingCounts"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Apply a grouping (summation of counts) across a set of detectors "
           "in Muon data.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess", "Minus", "Plus", "MuonGroupingAsymmetry", "MuonPairingAsymmetry", "MuonPreProcess"};
  }

  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  void setGroupingSampleLogs(const MatrixWorkspace_sptr &workspace);
};

} // namespace Muon
} // namespace Mantid
