// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <random>

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/HistogramY.h"

namespace Mantid::Algorithms {

/** CreateBootstrapWorkspaces: The algorithm generates several simulated workspaces by sampling
randomly from the input data, useful for doing a Bootstrap Analysis.
Two kinds of sampling are provided:
  - Sampling from a gaussian at each data point: y = Gaussian(y, e)
  - Sampling entire spectra with repetition: bootWs.Y(index) = origWs.Y(Uniform(0, nHist))
 */
class MANTID_ALGORITHMS_DLL CreateBootstrapWorkspaces : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

  static HistogramData::HistogramY sampleHistogramFromGaussian(const HistogramData::HistogramY &dataY,
                                                               const HistogramData::HistogramE &dataE,
                                                               std::mt19937 &gen);

private:
  void init() override;
  void exec() override;
};

} // namespace Mantid::Algorithms
