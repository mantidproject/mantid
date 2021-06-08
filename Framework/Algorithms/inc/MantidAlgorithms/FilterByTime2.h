// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/DateAndTime.h"

namespace Mantid {
namespace Algorithms {

/** FilterByTime2 : TODO: DESCRIPTION

  @date 2012-04-25
*/
class MANTID_ALGORITHMS_DLL FilterByTime2 : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FilterByTime"; };
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Events\\EventFiltering"; }
  /// Algorithm's summary for identification overriding a virtual method
  const std::string summary() const override { return "Filter event data by time."; }

private:
  /// Sets documentation strings for this algorithm

  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
