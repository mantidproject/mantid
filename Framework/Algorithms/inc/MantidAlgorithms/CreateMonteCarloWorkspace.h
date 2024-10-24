// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
using namespace std;

/** CreateMonteCarloWorkspace : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL CreateMonteCarloWorkspace : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;


private:
  void init() override;
  void exec() override;

  void fillHistogramWithRandomData(auto &outputY, const std::vector<double> &cdf, int numIterations, std::mt19937 &gen);
  vector<double> computeNormalizedCDF(const auto &yData);
  int computeNumberOfIterations(const auto &yData);

};

} // namespace Algorithms
} // namespace Mantid
