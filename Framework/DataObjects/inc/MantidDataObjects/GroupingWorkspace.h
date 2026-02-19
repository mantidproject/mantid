// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataObjects {

/** A GroupingWorkspace is a subclass of Workspace2D
 * where each spectrum has a single number entry, the value
 * of which signifies to which group that workspace index belongs.
 *
 * Group numbers are counted starting at 1!
 *
 * @author Janik Zikovsky
 * @date 2011-05-02
 */
class MANTID_DATAOBJECTS_DLL GroupingWorkspace : public SpecialWorkspace2D {
public:
  GroupingWorkspace(const Geometry::Instrument_const_sptr &inst);
  GroupingWorkspace() = default;
  GroupingWorkspace(size_t numvectors);
  GroupingWorkspace &operator=(const GroupingWorkspace &) = delete;

  /// Returns a clone of the workspace
  std::unique_ptr<GroupingWorkspace> clone() const { return std::unique_ptr<GroupingWorkspace>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<GroupingWorkspace> cloneEmpty() const { return std::unique_ptr<GroupingWorkspace>(doCloneEmpty()); }

  /** Gets the name of the workspace type
  @return Standard string name  */
  const std::string id() const override { return "GroupingWorkspace"; }

  void makeDetectorIDToGroupMap(std::map<detid_t, int> &detIDToGroup, int64_t &ngroups) const;
  void makeDetectorIDToGroupVector(std::vector<int> &detIDToGroup, int64_t &ngroups) const;
  int getTotalGroups() const;
  std::vector<int> getGroupIDs(const bool includeUnsetGroup = true) const;
  std::vector<detid_t> getDetectorIDsOfGroup(const int groupID) const;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  GroupingWorkspace(const GroupingWorkspace &) = default;

private:
  int translateToGroupID(const int n) const;
  GroupingWorkspace *doClone() const override { return new GroupingWorkspace(*this); }
  GroupingWorkspace *doCloneEmpty() const override { return new GroupingWorkspace(); }
};

/// shared pointer to the GroupingWorkspace class
using GroupingWorkspace_sptr = std::shared_ptr<GroupingWorkspace>;

/// shared pointer to a const GroupingWorkspace
using GroupingWorkspace_const_sptr = std::shared_ptr<const GroupingWorkspace>;

} // namespace DataObjects
} // namespace Mantid

#ifndef DataObjects_EXPORTS
#include "MantidAPI/WorkspaceProperty.h"
namespace Mantid::API {
/// @cond
extern template class MANTID_DATAOBJECTS_DLL WorkspaceProperty<DataObjects::GroupingWorkspace>;
/// @endcond
} // namespace Mantid::API
#endif
