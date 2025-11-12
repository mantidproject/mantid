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

using namespace Mantid::Types::Core;

/** Takes an existing sample log, and calculates its first or second
 * derivative, and adds it as a new log.

  @author Reece Boston
  @date Nov 11, in the two-thouandth and twenty fifth year of our Lord
*/

class MANTID_ALGORITHMS_DLL AddLogSmoothed final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "AddLogSmoothed"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Add a sample log that is the smoothed version of an existing log";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"AddSampleLog", "AddLogDerivative"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

  std::vector<double> getUniformXValues(std::vector<double> const &x);
  std::vector<double> getSplinedYValues(std::vector<double> const &nx, std::vector<double> const &x,
                                        std::vector<double> const &y);
  std::vector<DateAndTime> timesToDateAndTime(DateAndTime const &start, std::vector<double> const &times);

private:
  /// Initialise the properties
  void init() override;
  /// Validate input parameters
  std::map<std::string, std::string> validateInputs() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
