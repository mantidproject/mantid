// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** Filters events in an EventWorkspace using values in a SampleLog.
 */
class MANTID_ALGORITHMS_DLL FilterByLogValue : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FilterByLogValue"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Filter out events from an EventWorkspace based on a sample log "
           "value satisfying filter criteria.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"FilterByXValue", "FilterEvents", "FilterLogByTime", "FilterBadPulses", "FilterByTime"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events\\EventFiltering"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
