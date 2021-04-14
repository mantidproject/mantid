// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** StripVanadiumPeaks2

  @author Wenduo Zhou
  @date 2011-10-07
*/
class MANTID_ALGORITHMS_DLL StripVanadiumPeaks2 : public API::ParallelAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "StripVanadiumPeaks"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"FindPeaks", "StripPeaks"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "CorrectionFunctions\\PeakCorrections;Optimization\\PeakFinding;"
           "Diffraction\\Corrections";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm removes peaks (at vanadium d-spacing positions by "
           "default)"
           " out of a background by linearly/quadratically interpolating over "
           "the expected peak positions. ";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
