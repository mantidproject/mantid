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

/** CreateMonteCarloWorkspace : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL CreateMonteCarloWorkspace : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

  void afterPropertySet(const std::string &name) override;

private:
  void init() override;
  void exec() override;

  double calculateMean(const std::vector<int> &numbers);
  double calculateVariance(const std::vector<int> &numbers, double mean);
  std::vector<double> pdf(const std::vector<int> &num, double mean, double variance) const;
};

} // namespace Algorithms
} // namespace Mantid
