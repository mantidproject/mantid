// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <random>
#include <vector>

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/HistogramY.h"

namespace Mantid {
namespace Algorithms {
using namespace std;

/** CreateMonteCarloWorkspace : The algorithm generates a simulated workspace by sampling from the probability
distribution of input data, useful for testing of fitting functions and modeling.
 */
class MANTID_ALGORITHMS_DLL CreateBootstrapWorkspaces : public API::Algorithm {
public:
  const string name() const override;
  int version() const override;
  const string category() const override;
  const string summary() const override;

  static HistogramData::HistogramY sampleHistogramFromGaussian(const HistogramData::HistogramY &dataY,
                                                               const HistogramData::HistogramE &dataE,
                                                               std::mt19937 &gen);

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
