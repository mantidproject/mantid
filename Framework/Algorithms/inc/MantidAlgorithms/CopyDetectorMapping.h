// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/**
    Algorithm to copy spectra-detector mapping from one matrix workspace
    to another.

    @author Dan Nixon
*/
class MANTID_ALGORITHMS_DLL CopyDetectorMapping : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CopyDetectorMapping"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Copies spectra to detector mapping from one Matrix Workspace to "
           "another.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"CopyLogs"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Utility\\Workspaces"; }

  /// Input property validation
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
