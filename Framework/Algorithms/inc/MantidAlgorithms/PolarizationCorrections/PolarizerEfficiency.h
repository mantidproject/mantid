// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include <map>
#include <string>

namespace Mantid {
namespace Algorithms {
using namespace API;

class MANTID_ALGORITHMS_DLL PolarizerEfficiency final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PolarizerEfficiency"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates the efficiency of a polarizer."; }
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
  void calculatePolarizerEfficiency();
  void calculateErrors(const MatrixWorkspace_sptr &t00Ws, const MatrixWorkspace_sptr &t01Ws,
                       const MatrixWorkspace_sptr &effCellWs, const MatrixWorkspace_sptr &effPolarizerWs);
  void saveToFile(MatrixWorkspace_sptr const &workspace, std::string const &filePathStr);
};

} // namespace Algorithms
} // namespace Mantid