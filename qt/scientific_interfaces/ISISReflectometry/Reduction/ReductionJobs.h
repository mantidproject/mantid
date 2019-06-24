// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#include "Common/DllConfig.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include <boost/optional.hpp>

#include "Group.h"

namespace MantidQt {
namespace CustomInterfaces {

/** @class ReductionJobs

    The ReductionJobs model holds information about all jobs to be performed as
    part of a batch reduction.
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs {
public:
  ReductionJobs();
  explicit ReductionJobs(std::vector<Group> groups);
  Group &appendGroup(Group group);
  Group &insertGroup(Group group, int beforeIndex);
  bool hasGroupWithName(std::string const &groupName) const;
  bool containsSingleEmptyGroup() const;
  boost::optional<int> indexOfGroupWithName(std::string const &groupName) const;
  void removeGroup(int index);
  void removeAllGroups();
  void resetState();
  void resetSkippedItems();

  std::vector<Group> &mutableGroups();
  std::vector<Group> const &groups() const;
  Group const &operator[](int index) const;
  std::string nextEmptyGroupName();

  MantidWidgets::Batch::RowPath getPath(Group const &group) const;
  MantidWidgets::Batch::RowPath getPath(Row const &row) const;
  Group const &getParentGroup(Row const &row) const;
  boost::optional<Item &>
  getItemWithOutputWorkspaceOrNone(std::string const &wsName);

  bool
  validItemAtPath(const MantidWidgets::Batch::RowLocation rowLocation) const;
  Group const &
  getGroupFromPath(const MantidWidgets::Batch::RowLocation path) const;
  boost::optional<Row> const &
  getRowFromPath(const MantidWidgets::Batch::RowLocation path) const;
  Item const &
  getItemFromPath(const MantidWidgets::Batch::RowLocation path) const;

private:
  std::vector<Group> m_groups;
  size_t m_groupNameSuffix;
};

MANTIDQT_ISISREFLECTOMETRY_DLL bool operator!=(ReductionJobs const &lhs,
                                               ReductionJobs const &rhs);
MANTIDQT_ISISREFLECTOMETRY_DLL bool operator==(ReductionJobs const &lhs,
                                               ReductionJobs const &rhs);

void appendEmptyRow(ReductionJobs &jobs, int groupIndex);
void insertEmptyRow(ReductionJobs &jobs, int groupIndex, int beforeRow);
void removeRow(ReductionJobs &jobs, int groupIndex, int rowIndex);
void updateRow(ReductionJobs &jobs, int groupIndex, int rowIndex,
               boost::optional<Row> const &newValue);

void appendEmptyGroup(ReductionJobs &jobs);
void insertEmptyGroup(ReductionJobs &jobs, int beforeGroup);
bool hasGroupsWithContent(ReductionJobs const &jobs);
void ensureAtLeastOneGroupExists(ReductionJobs &jobs);
void removeGroup(ReductionJobs &jobs, int groupIndex);
void removeAllRowsAndGroups(ReductionJobs &jobs);

bool setGroupName(ReductionJobs &jobs, int groupIndex,
                  std::string const &newValue);
std::string groupName(ReductionJobs const &jobs, int groupIndex);

int percentComplete(ReductionJobs const &jobs);

void mergeRowIntoGroup(ReductionJobs &jobs, Row const &row,
                       double thetaTolerance, std::string const &groupName,
                       bool (*rowChanged)(Row const &rowA, Row const &rowB));

template <typename ModificationListener>
void mergeJobsInto(ReductionJobs &intoHere, ReductionJobs const &fromHere,
                   double thetaTolerance, ModificationListener &listener) {
  // If there's a "fake" empty group, then we want to remove it
  auto removeFirstGroup = intoHere.containsSingleEmptyGroup();
  for (auto const &group : fromHere.groups()) {
    auto maybeGroupIndex = intoHere.indexOfGroupWithName(group.name());
    if (maybeGroupIndex.is_initialized()) {
      auto indexToUpdateAt = maybeGroupIndex.get();
      auto &intoGroup = intoHere.mutableGroups()[indexToUpdateAt];
      mergeRowsInto(intoGroup, group, indexToUpdateAt, thetaTolerance,
                    listener);
    } else {
      intoHere.appendGroup(group);
      listener.groupAppended(static_cast<int>(intoHere.groups().size()) - 1,
                             group);
    }
  }
  // Remove the fake group after we have added the content, otherwise the
  // JobTreeView will add another fake group
  if (removeFirstGroup) {
    intoHere.removeGroup(0);
    listener.groupRemoved(0);
  }
}
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
