// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Renames a workspace to a different name in the data service.
    If the same name is provided for input and output then the algorithm will
   fail with an error.
    The renaming is implemented as a removal of the original workspace from the
   data service
    and re-addition under the new name.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the to rename the workspace to </LI>
    </UL>
 */
class MANTID_ALGORITHMS_DLL RenameWorkspace : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "RenameWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Rename the Workspace."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"RenameWorkspaces"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Utility\\Workspaces"; }
  /// Check that input params are valid
  std::map<std::string, std::string> validateInputs() override;

private:
  const std::string workspaceMethodName() const override { return "rename"; }
  const std::string workspaceMethodInputProperty() const override { return "InputWorkspace"; }

  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  bool processGroups() override;
};

} // namespace Algorithms
} // namespace Mantid
