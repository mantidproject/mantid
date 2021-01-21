// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {

namespace Algorithms {

/** Filters an EventWorkspace by wall-clock time, and outputs to a new event
   workspace
    or replaces the existing one.

    @author Janik Zikovsky, SNS
    @date September 14th, 2010
*/
class MANTID_ALGORITHMS_DLL FilterByTime : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FilterByTime"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm filters out events from an EventWorkspace that are "
           "not between given start and stop times.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadEventNexus", "FilterByXValue", "FilterEvents", "FilterLogByTime", "FilterBadPulses"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events\\EventFiltering"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  /// Pointer for an event workspace
  DataObjects::EventWorkspace_const_sptr eventW;
};

} // namespace Algorithms
} // namespace Mantid
