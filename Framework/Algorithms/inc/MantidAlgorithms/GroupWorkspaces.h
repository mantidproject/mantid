#ifndef MANTID_ALGORITHM_GROUP_H_
#define MANTID_ALGORITHM_GROUP_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

namespace Mantid {
namespace Algorithms {

/** Takes   workspaces as input and groups the workspaces.

Required Properties:
<UL>
<LI> InputWorkspaces - The name of the workspaces to group  </LI>
<LI> OutputWorkspace - The name of the new group workspace created </LI>
</UL>

Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport GroupWorkspaces : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GroupWorkspaces"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Takes workspaces as input and groups similar workspaces together.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"UnGroupWorkspace"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Transforms\\Grouping;Utility\\Workspaces";
  }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  /// Overridden Init method
  void init() override;
  /// overridden execute method
  void exec() override;
  /// Add a list of names to the new group
  void addToGroup(const std::vector<std::string> &names);
  /// Add a workspace to the new group, checking for a WorkspaceGroup and
  /// unrolling it
  void addToGroup(const API::Workspace_sptr &workspace);

  /// A pointer to the new group
  API::WorkspaceGroup_sptr m_group;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REGROUP_H_*/
