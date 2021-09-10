// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** ResetNegatives : Reset negative values to something else.

  @date 2012-02-01
*/
class MANTID_ALGORITHMS_DLL ResetNegatives : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Reset negative values to something else."; }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void pushMinimum(const API::MatrixWorkspace_const_sptr &minWS, const API::MatrixWorkspace_sptr &wksp,
                   API::Progress &prog);
  void changeNegatives(const API::MatrixWorkspace_const_sptr &minWS, const double spectrumNegativeValues,
                       const API::MatrixWorkspace_sptr &wksp, API::Progress &prog);
};

} // namespace Algorithms
} // namespace Mantid
