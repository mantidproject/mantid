// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Algorithms {

/** Takes   workspaces as input and groups the workspaces.

Required Properties:
<UL>
<LI> InputWorkspaces - The name of the workspaces to group  </LI>
<LI> OutputWorkspace - The name of the new group workspace created </LI>
</UL>
 */
class MANTID_ALGORITHMS_DLL GroupWorkspaces final : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GroupWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes workspaces as input and groups similar workspaces together.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"UnGroupWorkspace"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Grouping;Utility\\Workspaces"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Overridden Init method
  void init() override;
  /// overridden execute method
  void exec() override;
  /// Add a list of names to the new group
  void addToGroup(const std::vector<std::string> &names, const std::string &outputName = "");
  /// Add a workspace to the new group, checking for a WorkspaceGroup and
  /// unrolling it
  void addToGroup(const API::Workspace_sptr &workspace);
  /// Use a glob pattern to select workspaces in the ADS
  void addToGroup(const std::string &globExpression);
  /// A pointer to the new group
  API::WorkspaceGroup_sptr m_group;
};

} // namespace Algorithms
} // namespace Mantid
