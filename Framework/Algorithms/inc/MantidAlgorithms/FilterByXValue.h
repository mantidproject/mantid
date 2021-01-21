// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** Filters the events in an event workspace according to a minimum and/or
   maximum
    value of X. The filter limits should be given in whatever the units of the
   input
    workspace are (e.g. TOF).
*/
class MANTID_ALGORITHMS_DLL FilterByXValue : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Filters events according to a min and/or max value of X."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"FilterByTime", "FilterByLogValue", "FilterBadPulses"};
  }
  const std::string category() const override;

  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
