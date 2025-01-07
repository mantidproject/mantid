// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include <memory>
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
 */
class MANTID_API_DLL MultiPeriodGroupWorker {
public:
  /// Convenience typdef for workspace names.
  using VecWSGroupType = std::vector<WorkspaceGroup_sptr>;
  /// Constructor
  MultiPeriodGroupWorker() = default;
  /// Constructor
  MultiPeriodGroupWorker(std::string workspacePropertyName);
  virtual ~MultiPeriodGroupWorker() = default;
  /// Flag to indicate use of a custom workspace property
  bool useCustomWorkspaceProperty() const;
  /// Check groups
  VecWSGroupType findMultiPeriodGroups(Algorithm const *const sourceAlg) const;
  /// Process groups
  bool processGroups(Algorithm *const sourceAlg, const VecWSGroupType &vecMultiPeriodGroups) const;

private:
  // Disable copy
  MultiPeriodGroupWorker(const MultiPeriodGroupWorker &);
  // Disable assignment
  MultiPeriodGroupWorker &operator=(const MultiPeriodGroupWorker &);

  /// Try ot add a workspace to the group of input workspaces.
  void tryAddInputWorkspaceToInputGroups(const Workspace_sptr &ws, VecWSGroupType &vecMultiPeriodWorkspaceGroups,
                                         VecWSGroupType &vecWorkspaceGroups) const;

  /// Copy input workspace properties to spawned algorithm.
  void copyInputWorkspaceProperties(IAlgorithm *targetAlg, IAlgorithm *sourceAlg, const int &periodNumber) const;

  /// Create an input workspace string from the workspace groups.
  std::string createFormattedInputWorkspaceNames(const size_t &periodIndex,
                                                 const VecWSGroupType &vecWorkspaceGroups) const;

  /// Validate the input group workspace
  void validateMultiPeriodGroupInputs(const VecWSGroupType &vecMultiPeriodGroups) const;

  /// Workspace property name
  std::string m_workspacePropertyName;
};

} // namespace API
} // namespace Mantid
