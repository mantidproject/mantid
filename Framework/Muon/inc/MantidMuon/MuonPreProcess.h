// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMuon/DllConfig.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Muon {

class MANTID_MUON_DLL MuonPreProcess final : public API::Algorithm {
public:
  MuonPreProcess() : API::Algorithm() {}

  const std::string name() const override { return "MuonPreProcess"; }
  int version() const override { return (1); }
  const std::string category() const override { return "Muon\\DataHandling"; }
  const std::string summary() const override {
    return "Perform a series of common analysis pre-processing steps on Muon "
           "data. Sample logs are modified to record the input parameters.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"MuonProcess", "ApplyDeadTimeCorr",     "ChangeBinOffset",    "CropWorkspace",
            "Rebin",       "MuonGroupingAsymmetry", "MuonGroupingCounts", "MuonPairingAsymmetry"};
  }

private:
  void init() override;
  void exec() override;

  /// Apply a series of corrections ; DTC, offset, rebin, crop
  WorkspaceGroup_sptr correctWorkspaces(const WorkspaceGroup_sptr &wsGroup);
  MatrixWorkspace_sptr correctWorkspace(MatrixWorkspace_sptr ws);

  MatrixWorkspace_sptr applyDTC(MatrixWorkspace_sptr ws, const TableWorkspace_sptr &dt);

  MatrixWorkspace_sptr applyTimeOffset(MatrixWorkspace_sptr ws, const double &offset);

  MatrixWorkspace_sptr applyTimeZeroTable(const MatrixWorkspace_sptr &ws, const TableWorkspace_sptr &tz);

  MatrixWorkspace_sptr applyCropping(MatrixWorkspace_sptr ws, const double &xMin, const double &xMax);

  MatrixWorkspace_sptr applyRebinning(MatrixWorkspace_sptr ws, const std::vector<double> &rebinArgs);

  MatrixWorkspace_sptr cloneWorkspace(const MatrixWorkspace_sptr &ws);

  /// Add the correction inputs into the logs
  void addPreProcessSampleLogs(const WorkspaceGroup_sptr &group);

  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;

  /// Validates the tables used in the alg (called in validateInputs)
  void validateTableInputs(std::map<std::string, std::string> &errors);

  /// Allow WorkspaceGroup property to function correctly.
  bool checkGroups() override;

  /// Crop workspace with single xMin and xMax values
  MatrixWorkspace_sptr cropWithSingleValues(const MatrixWorkspace_sptr &ws, const double xMin, const double xMax);

  /// Crop workspace with vector of doubles
  MatrixWorkspace_sptr cropWithVectors(const MatrixWorkspace_sptr &ws, const double xMin, const double xMax);
};

} // namespace Muon
} // namespace Mantid
