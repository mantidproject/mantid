// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DataProcessorAlgorithm.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Workflow algorithm to compute a patched sensitivity correction for EQSANS.
*/
class DLLExport ComputeSensitivity : public API::DataProcessorAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ComputeSensitivity"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Workflow to calculate EQSANS sensitivity correction."; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "Workflow\\SANS\\UsesPropertyManager"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
