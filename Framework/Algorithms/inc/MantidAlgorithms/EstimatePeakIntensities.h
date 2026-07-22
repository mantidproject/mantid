// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

#include <map>
#include <string>
#include <vector>

namespace Mantid::Algorithms {

/** EstimatePeakIntensities : fit-independent, per-spectrum estimate of a peak's background and
  integrated intensity over a window.

  For every spectrum of the input workspace the algorithm integrates (data - background) over a
  per-spectrum X window.  The background is the mean of the "background" points chosen by the
  skew-seed method - the largest points are peeled off the window one at a time while the third
  central moment of the remaining points keeps decreasing and stays non-negative; the points left
  over are the background.  This reproduces the estimate in
  Engineering/texture/TextureUtils/fitting_utils.py (used as a fit-independent cross-check of the
  fitted peak area) but runs in C++ and parallelises over spectra with OpenMP, mirroring FitPeaks.
*/
class MANTID_ALGORITHMS_DLL EstimatePeakIntensities : public API::Algorithm {
public:
  const std::string name() const override { return "EstimatePeakIntensities"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Optimization\\PeakFinding"; }
  const std::string summary() const override {
    return "Fit-independent per-spectrum estimate of a peak's background and integrated intensity over a window.";
  }
  const std::vector<std::string> seeAlso() const override { return {"FitPeaks", "FindPeakBackground"}; }

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
};

} // namespace Mantid::Algorithms
