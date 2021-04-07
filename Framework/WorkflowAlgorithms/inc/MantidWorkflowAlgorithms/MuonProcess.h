// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/** MuonProcess : Processes and analyses Muon workspace.
 */
class DLLExport MuonProcess : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Processes and analyses Muon workspace."; }

  int version() const override;
  const std::string category() const override;

  /// Perform validation of inputs to the algorithm
  std::map<std::string, std::string> validateInputs() override;

private:
  void init() override;
  void exec() override;

  // We dont' want processGroups to be called
  bool checkGroups() override { return false; }

  /// Groups specified workspace group according to specified
  /// DetectorGroupingTable.
  API::WorkspaceGroup_sptr groupWorkspaces(const API::WorkspaceGroup_sptr &wsGroup,
                                           const DataObjects::TableWorkspace_sptr &grouping);

  /// Applies dead time correction to the workspace group.
  API::WorkspaceGroup_sptr applyDTC(const API::WorkspaceGroup_sptr &wsGroup,
                                    const DataObjects::TableWorkspace_sptr &dt);

  /// Applies offset, crops and rebin the workspace according to specified
  /// params
  API::MatrixWorkspace_sptr correctWorkspace(API::MatrixWorkspace_sptr ws, double loadedTimeZero);

  /// Applies offset, crops and rebins all workspaces in the group
  API::WorkspaceGroup_sptr correctWorkspaces(const API::WorkspaceGroup_sptr &wsGroup, double loadedTimeZero);

  /// Builds an error message from a list of invalid periods
  std::string buildErrorString(const std::vector<int> &invalidPeriods) const;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
