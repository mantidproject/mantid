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
  /// Calculate the efficiency of the helium analyser from the measured transmission data
  MatrixWorkspace_sptr calculateAnalyserEfficiency();
  /// Fit the measured efficiency to the known theoretical relationship of efficiency=(1 + tanh(mu * pHe *
  /// wavelength))/2 to find pHe and pHeError
  void fitAnalyserEfficiency(const double mu, const MatrixWorkspace_sptr eff, double &pHe, double &pHeError);
  /// Use the relationship efficiency=(1 + tanh(mu * pHe * wavelength))/2 to calculate the theoretical efficiencies for
  /// the wavelength bins in the given MatrixWorkspace.
  void convertToTheoreticalEfficiency(MatrixWorkspace_sptr &eff, const double pHe, const double pHeError,
                                      const double mu);
  /// Calculates the tCrit value to give us correct error bounds
  double calculateTCrit(const size_t numberOfBins);

  static const double ABSORPTION_CROSS_SECTION_CONSTANT;
};
} // namespace Algorithms
} // namespace Mantid
