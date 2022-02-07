// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidMDAlgorithms/ImportMDHistoWorkspaceBase.h"

namespace Mantid {
namespace MDAlgorithms {

/** ImportMDHistoWorkspace : Takes a text file containing structured signal and
  error information and imports it
  as a new MDHistoWorkspace.

  @date 2012-06-20
*/
class DLLExport ImportMDHistoWorkspace : public ImportMDHistoWorkspaceBase {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Reads a text file and generates an MDHistoWorkspace from it."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ImportMDEventWorkspace"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace MDAlgorithms
} // namespace Mantid
