#ifndef MANTID_API_MULTIPERIODGROUPWORKER_H_
#define MANTID_API_MULTIPERIODGROUPWORKER_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace Mantid {
namespace API {

/** MultiPeriodGroupWorker : Multiperiod group logic relating to determining a
 valid multiperiod group, and processing a
 * multiperiod group, as well as combining and returning the output.
 *
 *
 * Determines if the input workspaces are multiperiod group workspaces
 * Processes the multiperiod group workspaces period by period running a new
 instance of the target algorithm for each one, then regrouping the results
 *
 *

 Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MultiPeriodGroupWorker {
public:
  /// Convenience typdef for workspace names.
  using VecWSGroupType = std::vector<WorkspaceGroup_sptr>;
  /// Constructor
  MultiPeriodGroupWorker() = default;
  /// Constructor
  MultiPeriodGroupWorker(const std::string &workspacePropertyName);
  virtual ~MultiPeriodGroupWorker() = default;
  /// Flag to indicate use of a custom workspace property
  bool useCustomWorkspaceProperty() const;
  /// Check groups
  VecWSGroupType findMultiPeriodGroups(Algorithm const *const sourceAlg) const;
  /// Process groups
  bool processGroups(Algorithm *const sourceAlg,
                     const VecWSGroupType &vecMultiPeriodGroups) const;

private:
  // Disable copy
  MultiPeriodGroupWorker(const MultiPeriodGroupWorker &);
  // Disable assignment
  MultiPeriodGroupWorker &operator=(const MultiPeriodGroupWorker &);

  /// Try ot add a workspace to the group of input workspaces.
  void tryAddInputWorkspaceToInputGroups(
      Workspace_sptr ws, VecWSGroupType &vecMultiPeriodWorkspaceGroups,
      VecWSGroupType &vecWorkspaceGroups) const;

  /// Copy input workspace properties to spawned algorithm.
  void copyInputWorkspaceProperties(IAlgorithm *targetAlg,
                                    IAlgorithm *sourceAlg,
                                    const int &periodNumber) const;

  /// Create an input workspace string from the workspace groups.
  std::string createFormattedInputWorkspaceNames(
      const size_t &periodIndex,
      const VecWSGroupType &vecWorkspaceGroups) const;

  /// Validate the input group workspace
  void validateMultiPeriodGroupInputs(
      const VecWSGroupType &vecMultiPeriodGroups) const;

  /// Workspace property name
  std::string m_workspacePropertyName;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MULTIPERIODGROUPWORKER_H_ */
