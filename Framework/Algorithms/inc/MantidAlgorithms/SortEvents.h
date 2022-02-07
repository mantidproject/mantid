// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Takes an EventWorkspace and sorts by TOF or frame_index.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to take as input. Must
   contain event data. </LI>
    <LI> SortByTof - check to sort by Time of Flight; uncheck to sort by frame
   index.</LI>
    </UL>

    @author Janik Zikovsky, SNS
    @date Friday, August 13, 2010.
 */
class MANTID_ALGORITHMS_DLL SortEvents : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SortEvents"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Sort the events in an EventWorkspace, for faster rebinning."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadEventNexus"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events;Utility\\Sorting"; }

protected:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
