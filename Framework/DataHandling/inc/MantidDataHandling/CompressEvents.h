// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace DataHandling {
/** Compress an EventWorkspace by lumping together events with very close TOF
 value,
 * while ignoring the event's pulse time.
 *
 * This algorithm will go through all event lists and sum up together the
 weights and errors
 * of events with times-of-flight within a specified tolerance.
 * The event list data type is converted to WeightedEventNoTime, where the pulse
 time information
 * is not saved, in order to save memory.

    @author Janik Zikovsky, SNS
    @date Jan 19, 2011
*/
class DLLExport CompressEvents : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CompressEvents"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Reduce the number of events in an EventWorkspace by grouping "
           "together events with identical or similar X-values "
           "(time-of-flight).";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LoadEventNexus", "LoadEventAndCompress"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
