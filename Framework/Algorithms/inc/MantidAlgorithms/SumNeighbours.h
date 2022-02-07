// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Sums neighboring pixels on rectangular detectors.
 * Each spectrum in the output workspace is a sum of a block of SumX*SumY
 pixels.
 * Only works on EventWorkspaces and for instruments with RectangularDetector's.
 *
 * This only works for instruments that have RectangularDetector's defined;
 * at the time of writing: TOPAZ, SNAP, PG3.
 *
    @author Janik Zikovsky, SNS
    @date Oct 2010
 */
class MANTID_ALGORITHMS_DLL SumNeighbours : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SumNeighbours"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sum event lists from neighboring pixels in rectangular area "
           "detectors - e.g. to reduce the signal-to-noise of individual "
           "spectra. Each spectrum in the output workspace is a sum of a block "
           "of SumX*SumY pixels. Only works on EventWorkspaces and for "
           "instruments with RectangularDetector's.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SumSpectra", "SumRowColumn"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Grouping"; }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
