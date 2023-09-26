// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** RebinRagged : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL RebinRagged : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override { return {"Rebin", "ResampleX"}; }

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;
  static bool use_simple_rebin(std::vector<double> xmins, std::vector<double> xmaxs, std::vector<double> deltas);
  static void extend_value(size_t numSpec, std::vector<double> &array);
};

} // namespace Algorithms
} // namespace Mantid
