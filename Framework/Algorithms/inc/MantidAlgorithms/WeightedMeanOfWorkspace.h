// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** WeightedMeanOfWorkspace This algorithm calculates the weighted mean for
 * a single workspace from all the detector (non-monitor, not masked) spectra
 * in that workspace. The results is a single value for the entire workspace.
 */
class MANTID_ALGORITHMS_DLL WeightedMeanOfWorkspace : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm calculates the weighted mean for an entire "
           "workspace.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Mean", "WeightedMean"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
