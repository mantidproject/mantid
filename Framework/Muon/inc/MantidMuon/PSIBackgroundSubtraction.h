// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Muon {

class MANTID_MUON_DLL PSIBackgroundSubtraction : public API::DataProcessorAlgorithm {

public:
  PSIBackgroundSubtraction() : API::DataProcessorAlgorithm() {}
  const std::string name() const override { return "PSIBackgroundSubtraction"; }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Fit", "LoadPSIMuonBin"}; }
  const std::string summary() const override { return "Removes the background from a loaded PSI workspace."; }
  const std::string category() const override { return "Muon"; }

private:
  void init() override;
  void exec() override;
  void calculateBackgroundUsingFit(API::MatrixWorkspace_sptr &inputWorkspace);
  /// setup the child fit algorithm, can be overwritten by the tests with a
  /// mock.
  API::IAlgorithm_sptr setupFitAlgorithm(const std::string &wsName);
  virtual std::tuple<double, double> calculateBackgroundFromFit(API::IAlgorithm_sptr &fit,
                                                                const std::pair<double, double> &range,
                                                                const int &workspaceIndex);

  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;
};
} // namespace Muon
} // namespace Mantid