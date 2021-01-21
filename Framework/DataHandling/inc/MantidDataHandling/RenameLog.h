// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

/** RenameLog : TODO: DESCRIPTION

  @date 2011-12-16
*/
class DLLExport RenameLog : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "RenameLog"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Rename a TimeSeries log in a given Workspace."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"RemoveLogs", "DeleteLog"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Logs"; }

private:
  void init() override;

  void exec() override;

  API::MatrixWorkspace_sptr matrixWS;
};

} // namespace DataHandling
} // namespace Mantid
