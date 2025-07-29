// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
using namespace API;
using VectorPair = std::pair<std::vector<double>, std::vector<double>>;
/*
Docs
*/
class MANTID_ALGORITHMS_DLL HeliumAnalyserEfficiency final : public API::Algorithm {
public:
  HeliumAnalyserEfficiency();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "HeliumAnalyserEfficiency"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates the efficiency of a He3 analyser."; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "SANS\\PolarizationCorrections"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  void declareInputProperties();
  void declareFitProperties();
  void declareOutputProperties();

  void fitDecayTime(const MatrixWorkspace_sptr &workspace);
  void makeFit(const Algorithm_sptr &fitAlgorithm, const std::string &fitOutputName);
  void prepareOutputs(const std::vector<MatrixWorkspace_sptr> &efficiencies);
  std::vector<MatrixWorkspace_sptr> calculateEfficiencies(const std::vector<std::string> &workspaceNames,
                                                          const std::string &spinConfiguration);

  VectorPair fitHe3Polarization(const double mu, const std::vector<MatrixWorkspace_sptr> &efficiencies);
  VectorPair getTimeDifferences(const std::vector<std::string> &wsNames);

  void convertToTheoreticalEfficiencies(const std::vector<MatrixWorkspace_sptr> &efficiencies,
                                        const std::vector<double> &pHeVec, const std::vector<double> &pHeErrorVec,
                                        const double mu);
  double calculateTCrit(const size_t numberOfBins) const;

  std::vector<MatrixWorkspace_sptr> m_outputCurves;
  std::vector<ITableWorkspace_sptr> m_outputParameters;
};
} // namespace Algorithms
} // namespace Mantid
