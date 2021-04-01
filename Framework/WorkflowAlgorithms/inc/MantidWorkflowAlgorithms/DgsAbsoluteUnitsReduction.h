// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** DgsAbsoluteUnitsReduction : This is the algorithm responsible for
 * processing and creating the absolute units normalisation data.

@date 2012-11-10
 */
class DLLExport DgsAbsoluteUnitsReduction : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Process the absolute units sample."; }
  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
