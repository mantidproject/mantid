// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>
#include <random>

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"


namespace Mantid {
namespace Algorithms {
using namespace std;

/** CreateMonteCarloWorkspace : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL CreateMonteCarloWorkspace : public API::Algorithm {
public:
  const string name() const override;
  int version() const override;
  const string category() const override;
  const string summary() const override;

  Mantid::HistogramData::HistogramY fillHistogramWithRandomData(const std::vector<double> &cdf, int numIterations,
                                                                std::mt19937 &gen);
  std::vector<double> computeNormalizedCDF(const Mantid::HistogramData::HistogramY &yData);
  int computeNumberOfIterations(const Mantid::HistogramData::HistogramY &yData);

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
