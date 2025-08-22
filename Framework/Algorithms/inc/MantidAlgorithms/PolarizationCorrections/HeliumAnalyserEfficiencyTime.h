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
/*
Docs
*/
class MANTID_ALGORITHMS_DLL HeliumAnalyserEfficiencyTime final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "HeliumAnalyserEfficiencyTime"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the helium analyzer efficiency at the input workspace run time using the lifetime, initial "
           "polarization and mean gas "
           "length of the analyzer";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "SANS\\PolarizationCorrections"; }
  /// Related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"HeliumAnalyserEfficiency", "DepolarizedAnalyserTransmission"};
  };
  /// Check that input params are valid
  std::map<std::string, std::string> validateInputs() override;
  bool checkGroups() override { return false; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  std::pair<double, double> getTimeDifference();
  std::vector<MatrixWorkspace_sptr> calculateOutputs();
  MatrixWorkspace_sptr retrieveWorkspaceForWavelength() const;
};
} // namespace Algorithms
} // namespace Mantid
