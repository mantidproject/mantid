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

/** DgsRemap : This algorithm takes a workspace and masks and groups that
 * workspace if appropriate information is passed. It can be run in reverse
 * (group then mask) mode.
 */
class DLLExport DgsRemap : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Mask and/or group the given workspace."; }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
  void execGrouping(const API::MatrixWorkspace_sptr &iWS, API::MatrixWorkspace_sptr &oWS);
  void execMasking(const API::MatrixWorkspace_sptr &iWS);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
