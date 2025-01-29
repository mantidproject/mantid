// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

/** Takes an existing sample log, and calculates its first or second
 * derivative, and adds it as a new log.

  @author Janik Zikovsky
  @date 2011-09-16
*/
class MANTID_ALGORITHMS_DLL AddLogDerivative final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "AddLogDerivative"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Add a sample log that is the first or second derivative of an "
           "existing sample log.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"AddSampleLog"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

  static Mantid::Kernel::TimeSeriesProperty<double> *
  makeDerivative(API::Progress &progress, const Mantid::Kernel::TimeSeriesProperty<double> *input,
                 const std::string &name, int numDerivatives);

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
