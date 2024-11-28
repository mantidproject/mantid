// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <random>
#include <vector>

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
using namespace std;

/** CreateMonteCarloWorkspace : The algorithm generates a simulated workspace by sampling from the probability
distribution of input data, useful for testing of fitting functions and modeling.
By generating a simulated dataset that mirrors the probability
distribution of existing data.
 */
class MANTID_ALGORITHMS_DLL CreateMonteCarloWorkspace : public API::Algorithm {
public:
  const string name() const override;
  int version() const override;
  const string category() const override;
  const string summary() const override;

  Mantid::HistogramData::HistogramY fillHistogramWithRandomData(const std::vector<double> &cdf, int numIterations,
                                                                int seed_input, API::Progress &progress);
  std::vector<double> computeNormalizedCDF(const Mantid::HistogramData::HistogramY &yData);
  int computeNumberOfIterations(const Mantid::HistogramData::HistogramY &yData, int userMCEvents);
  Mantid::HistogramData::HistogramY scaleInputToMatchMCEvents(const Mantid::HistogramData::HistogramY &yData,
                                                              int targetMCEvents);

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
