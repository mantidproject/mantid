// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Takes an existing sample log and interpolates it according to the time axis of another log
  @author Reece Boston
  @date 2025-11-11
*/

class MANTID_ALGORITHMS_DLL AddLogInterpolated final : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "AddLogInterpolated"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Interpolates a given log to match a time series log's axis, using a cubic spline";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"AddSampleLog", "AddLogDerivative", "AddLogSmoothed"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

private:
  /// Initialise the properties
  void init() override;
  /// Validate input parameters
  std::map<std::string, std::string> validateInputs() override;
  /// Run the algorithm
  void exec() override;

  std::pair<size_t, size_t> findInterpolationRange(std::vector<double> const &, std::vector<double> const &) const;
};

} // namespace Algorithms
} // namespace Mantid
