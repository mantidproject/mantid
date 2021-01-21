// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** MaskBinsIf : Masks bins based on muparser expression
 */
class MANTID_ALGORITHMS_DLL MaskBinsIf : public API::Algorithm {
public:
  const std::string name() const override { return "MaskBinsIf"; }
  int version() const override { return 1; }
  const std::string category() const override { return "Transforms\\Masking"; }
  std::map<std::string, std::string> validateInputs() override;
  const std::string summary() const override { return "Masks bins based on muparser expression"; }
  const std::vector<std::string> seeAlso() const override { return {"MaskBins"}; }

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
