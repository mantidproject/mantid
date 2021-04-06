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

/** ParallaxCorrection : Performs geometrical correction for parallax effect in
 * tube based SANS instruments.
 */
class MANTID_ALGORITHMS_DLL ParallaxCorrection : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  void performCorrection(const API::MatrixWorkspace_sptr &, const std::vector<size_t> &, const std::string &,
                         const std::string &);
};

} // namespace Algorithms
} // namespace Mantid
