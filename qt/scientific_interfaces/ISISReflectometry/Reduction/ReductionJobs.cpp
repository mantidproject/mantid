// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionJobs.h"
#include "Common/IndexOf.h"
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "RowLocation.h"
#include <iostream>
#include <numeric>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
Group &findOrMakeGroupWithName(ReductionJobs &jobs, std::string const &groupName) {
  auto maybeGroupIndex = jobs.indexOfGroupWithName(groupName);
  if (maybeGroupIndex.has_value())
    return jobs.mutableGroups()[maybeGroupIndex.value()];
  else
    return jobs.appendGroup(Group(groupName));
}

/* Return the number of rows and groups that have processing or
 * postprocessing associated with them */
int countItems(ReductionJobs const &jobs, Item::ItemCountFunction countFunction) {
  auto const &groups = jobs.groups();
  return std::accumulate(groups.cbegin(), groups.cend(), 0, [countFunction](int const &count, Group const &group) {
    return count + (group.*countFunction)();
  });
}
} // namespace

ReductionJobs::ReductionJobs(std::vector<Group> groups) : m_groups(std::move(groups)), m_groupNameSuffix(1) {}

ReductionJobs::ReductionJobs() : m_groupNameSuffix(1) {}

Group &ReductionJobs::appendGroup(Group group) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  m_groups.emplace_back(std::move(group));
  return m_groups.back();
}

std::optional<int> ReductionJobs::indexOfGroupWithName(std::string const &groupName) const {
  return indexOf(m_groups, [&groupName](Group const &group) -> bool { return group.name() == groupName; });
}

Group &ReductionJobs::insertGroup(Group group, int beforeIndex) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  return *m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
}

bool ReductionJobs::hasGroupWithName(std::string const &groupName) const {
  return std::any_of(m_groups.crbegin(), m_groups.crend(),
                     [&groupName](Group const &group) -> bool { return group.name() == groupName; });
}

bool ReductionJobs::containsSingleEmptyGroup() const {
  return groups().size() == 1 && groups()[0].rows().size() == 0 && groups()[0].name().empty();
}

void ReductionJobs::removeGroup(int index) {
  m_groups.erase(m_groups.begin() + index);
  ensureAtLeastOneGroupExists(*this);
}

void ReductionJobs::removeAllGroups() {
  m_groups.clear();
  ensureAtLeastOneGroupExists(*this);
}

void ReductionJobs::resetState() {
  std::for_each(m_groups.begin(), m_groups.end(), [](Group &group) { group.resetState(); });
}

void ReductionJobs::resetSkippedItems() {
  std::for_each(m_groups.begin(), m_groups.end(), [](Group &group) { group.resetSkipped(); });
}

std::vector<Group> &ReductionJobs::mutableGroups() { return m_groups; }

std::vector<Group> const &ReductionJobs::groups() const { return m_groups; }

std::string ReductionJobs::nextEmptyGroupName() {
  std::string name = "HiddenGroupName" + std::to_string(m_groupNameSuffix);
  m_groupNameSuffix++;
  return name;
}

void ReductionJobs::setAllRowParents() {
  std::for_each(m_groups.begin(), m_groups.end(), [](auto &group) { group.setAllRowParents(); });
}

/* Return true if the reduction table has content. This excludes the
 * case where we have a single empty group that is usually a convenience
 * group that we've added to avoid an empty table so does not count as
 * user-entered content
 */
bool hasGroupsWithContent(ReductionJobs const &jobs) {
  if (jobs.groups().size() == 0)
    return false;

  if (jobs.containsSingleEmptyGroup())
    return false;

  return true;
}

/* This function is called after deleting groups to ensure that the model
 * always contains at least one group - it adds an empty group if
 * required. This is to mimic behaviour of the JobTreeView, which cannot delete
 * the last group/row so it always leaves at least one empty group.
 */
void ensureAtLeastOneGroupExists(ReductionJobs &jobs) {
  if (jobs.groups().size() == 0)
    appendEmptyGroup(jobs);
}

void removeGroup(ReductionJobs &jobs, int groupIndex) { jobs.removeGroup(groupIndex); }

void removeAllRowsAndGroups(ReductionJobs &jobs) { jobs.removeAllGroups(); }

void appendEmptyRow(ReductionJobs &jobs, int groupIndex) { jobs.mutableGroups()[groupIndex].appendEmptyRow(); }

void appendEmptyGroup(ReductionJobs &jobs) { jobs.appendGroup(Group(jobs.nextEmptyGroupName())); }

void insertEmptyGroup(ReductionJobs &jobs, int beforeGroup) {
  jobs.insertGroup(Group(jobs.nextEmptyGroupName()), beforeGroup);
}

void insertEmptyRow(ReductionJobs &jobs, int groupIndex, int beforeRow) {
  jobs.mutableGroups()[groupIndex].insertRow(boost::none, beforeRow);
}

void updateRow(ReductionJobs &jobs, int groupIndex, int rowIndex, boost::optional<Row> const &newValue) {
  if (newValue.is_initialized()) {
    jobs.mutableGroups()[groupIndex].updateRow(rowIndex, newValue);
  } else {
    jobs.mutableGroups()[groupIndex].updateRow(rowIndex, boost::none);
  }
}

void mergeRowIntoGroup(ReductionJobs &jobs, Row const &row, double thetaTolerance, std::string const &groupName) {
  auto &group = findOrMakeGroupWithName(jobs, groupName);
  auto indexOfRowToUpdate = group.indexOfRowWithTheta(row.theta(), thetaTolerance);

  if (indexOfRowToUpdate.has_value()) {
    auto rowToUpdate = group[indexOfRowToUpdate.value()].get();
    auto newRowValue = mergedRow(rowToUpdate, row);
    if (newRowValue.runNumbers() != rowToUpdate.runNumbers())
      group.updateRow(indexOfRowToUpdate.value(), newRowValue);
  } else {
    group.insertRowSortedByAngle(row);
  }
}

void removeRow(ReductionJobs &jobs, int groupIndex, int rowIndex) {
  jobs.mutableGroups()[groupIndex].removeRow(rowIndex);
}

bool setGroupName(ReductionJobs &jobs, int groupIndex, std::string const &newValue) {
  auto &group = jobs.mutableGroups()[groupIndex];
  if (group.name() != newValue) {
    if (newValue.empty() || !jobs.hasGroupWithName(newValue)) {
      group.setName(newValue);
    } else {
      return false;
    }
  }
  return true;
}

std::string groupName(ReductionJobs const &jobs, int groupIndex) { return jobs[groupIndex].name(); }

/* Return the percentage of items that have been completed */
int percentComplete(ReductionJobs const &jobs) {
  // If there's nothing to process we're 100% complete
  auto const total = countItems(jobs, &Item::totalItems);
  if (total == 0)
    return 100;

  auto const completed = countItems(jobs, &Item::completedItems);
  return static_cast<int>(completed * 100 / total);
}

Group const &ReductionJobs::operator[](int index) const { return m_groups[index]; }

MantidWidgets::Batch::RowLocation ReductionJobs::getLocation(Item const &item) const {
  if (item.isGroup())
    return getLocation(dynamic_cast<Group const &>(item));
  return getLocation(dynamic_cast<Row const &>(item));
}

MantidWidgets::Batch::RowLocation ReductionJobs::getLocation(Group const &group) const {
  // Find this group in the groups list
  auto groupIter = std::find_if(m_groups.cbegin(), m_groups.cend(),
                                [&group](Group const &currentGroup) -> bool { return &currentGroup == &group; });
  // Calling this function with a group that's not in the list is an error
  if (groupIter == m_groups.cend()) {
    throw std::runtime_error(std::string("Internal error: could not find table location for group ") + group.name());
  }
  // Found the group so return its index as the path
  auto const groupIndex = static_cast<int>(groupIter - m_groups.cbegin());
  return MantidWidgets::Batch::RowLocation({groupIndex});
}

MantidWidgets::Batch::RowLocation ReductionJobs::getLocation(Row const &row) const {
  auto groupIndex = 0;
  for (auto const &group : m_groups) {
    // See if the row is in this group
    auto const &rows = group.rows();
    auto rowIter = std::find_if(rows.cbegin(), rows.cend(), [&row](boost::optional<Row> const &currentRow) -> bool {
      return currentRow && &currentRow.get() == &row;
    });
    if (rowIter == rows.cend()) {
      // Try the next group
      ++groupIndex;
      continue;
    }

    // Found the row, so return its group and row indices as the path
    auto const rowIndex = static_cast<int>(rowIter - rows.cbegin());
    return MantidWidgets::Batch::RowLocation({groupIndex, rowIndex});
  }

  throw std::runtime_error("Internal error: could not find table location for row");
}

boost::optional<Item &> ReductionJobs::getItemWithOutputWorkspaceOrNone(std::string const &wsName) {
  for (auto &group : m_groups) {
    // Return this group if it has the output we're looking for
    if (group.postprocessedWorkspaceName() == wsName)
      return group;
    // If it has a child row with this workspace output, return it
    auto maybeRow = group.getItemWithOutputWorkspaceOrNone(wsName);
    if (maybeRow)
      return boost::optional<Item &>(maybeRow.get());
  }
  return boost::none;
}

Group const &ReductionJobs::getGroupFromPath(const MantidWidgets::Batch::RowLocation &rowLocation) const {
  if (isGroupLocation(rowLocation)) {
    return groups()[groupOf(rowLocation)];
  } else {
    throw std::invalid_argument("Path given does not point to a group.");
  }
}

boost::optional<Row> const &ReductionJobs::getRowFromPath(const MantidWidgets::Batch::RowLocation &rowLocation) const {
  if (isRowLocation(rowLocation)) {
    return groups()[groupOf(rowLocation)].rows()[rowOf(rowLocation)];
  } else {
    throw std::invalid_argument("Path given does not point to a row.");
  }
}

bool ReductionJobs::validItemAtPath(const MantidWidgets::Batch::RowLocation &rowLocation) const {
  if (isGroupLocation(rowLocation))
    return true;

  return getRowFromPath(rowLocation).is_initialized();
}

Item const &ReductionJobs::getItemFromPath(const MantidWidgets::Batch::RowLocation &rowLocation) const {
  if (isGroupLocation(rowLocation)) {
    return getGroupFromPath(rowLocation);
  } else {
    auto &maybeRow = getRowFromPath(rowLocation);
    if (!maybeRow.is_initialized())
      throw std::invalid_argument("Attempted to access invalid row");
    return maybeRow.get();
  }
}

bool operator!=(ReductionJobs const &lhs, ReductionJobs const &rhs) { return !(lhs == rhs); }

bool operator==(ReductionJobs const &lhs, ReductionJobs const &rhs) { return lhs.groups() == rhs.groups(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
