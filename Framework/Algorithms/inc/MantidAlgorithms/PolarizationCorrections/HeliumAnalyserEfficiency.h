// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {
using namespace API;
/*
Docs
*/
class MANTID_ALGORITHMS_DLL HeliumAnalyserEfficiency final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "HeliumAnalyserEfficiency"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates the efficiency of a He3 analyser."; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "SANS\\PolarizationCorrections"; }
  /// Check that input params are valid
  std::map<std::string, std::string> validateInputs() override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  void validateGroupInput();
  void calculateAnalyserEfficiency();
  MatrixWorkspace_sptr addTwoWorkspaces(MatrixWorkspace_sptr ws, MatrixWorkspace_sptr otherWs);
  MatrixWorkspace_sptr createWorkspace(const std::string &name, const std::string &title, const MantidVec &xData,
                                       const MantidVec &yData, const MantidVec &eData);
  MatrixWorkspace_sptr divideWorkspace(MatrixWorkspace_sptr numerator, MatrixWorkspace_sptr denominator);
  void fitAnalyserEfficiency(const double mu, MatrixWorkspace_sptr e, const MantidVec &wavelengthValues, double &pHe,
                             double &pHeError, MantidVec &eCalc);
  MatrixWorkspace_sptr calculateEfficiencyWorkspace(const MantidVec &wavelengthValues, const MantidVec &eValues,
                                                    const double pHe, const double pHeError, const double mu);
  double calculateTCrit(const size_t numberOfBins);

  static const double ABSORPTION_CROSS_SECTION_CONSTANT;
};
} // namespace Algorithms
} // namespace Mantid