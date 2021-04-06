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

/**
 * Add a peak to a PeaksWorkspace.

  @date 2012-10-16
 */
class MANTID_ALGORITHMS_DLL AddPeak : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "AddPeak"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Adds a peak to a PeaksWorkspace."; }
  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"AddPeakHKL", "CalculatePeaksHKL"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
