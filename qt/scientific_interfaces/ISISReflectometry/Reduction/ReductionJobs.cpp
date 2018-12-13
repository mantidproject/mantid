// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReductionJobs.h"
#include "../IndexOf.h"
#include "../Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

namespace {
Group &findOrMakeGroupWithName(ReductionJobs &jobs,
                               std::string const &groupName) {
  auto maybeGroupIndex = jobs.indexOfGroupWithName(groupName);
  if (maybeGroupIndex.is_initialized())
    return jobs.groups()[maybeGroupIndex.get()];
  else
    return jobs.appendGroup(Group(groupName));
} // unnamed
} // namespace

ReductionJobs::ReductionJobs(std::vector<Group> groups)
    : m_groups(std::move(groups)), m_groupNameSuffix(1) {}

ReductionJobs::ReductionJobs() : m_groupNameSuffix(1) {}

Group &ReductionJobs::appendGroup(Group group) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  m_groups.emplace_back(std::move(group));
  return m_groups.back();
}

boost::optional<int>
ReductionJobs::indexOfGroupWithName(std::string const &groupName) {
  return indexOf(m_groups, [&groupName](Group const &group) -> bool {
    return group.name() == groupName;
  });
}

Group &ReductionJobs::insertGroup(Group group, int beforeIndex) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  return *m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
}

bool ReductionJobs::hasGroupWithName(std::string const &groupName) const {
  return std::any_of(m_groups.crbegin(), m_groups.crend(),
                     [&groupName](Group const &group) -> bool {
                       return group.name() == groupName;
                     });
}

void ReductionJobs::removeGroup(int index) {
  m_groups.erase(m_groups.begin() + index);
}

std::vector<Group> &ReductionJobs::groups() { return m_groups; }

std::vector<Group> const &ReductionJobs::groups() const { return m_groups; }

std::string ReductionJobs::nextEmptyGroupName() {
  std::string name = "Group" + std::to_string(m_groupNameSuffix);
  m_groupNameSuffix++;
  return name;
}

void removeGroup(ReductionJobs &jobs, int groupIndex) {
  jobs.removeGroup(groupIndex);
}

void appendEmptyRow(ReductionJobs &jobs, int groupIndex) {
  jobs.groups()[groupIndex].appendEmptyRow();
}

void appendEmptyGroup(ReductionJobs &jobs) {
  jobs.appendGroup(Group(jobs.nextEmptyGroupName()));
}

void insertEmptyGroup(ReductionJobs &jobs, int beforeGroup) {
  jobs.insertGroup(Group(jobs.nextEmptyGroupName()), beforeGroup);
}

void insertEmptyRow(ReductionJobs &jobs, int groupIndex, int beforeRow) {
  jobs.groups()[groupIndex].insertRow(boost::none, beforeRow);
}

void updateRow(ReductionJobs &jobs, int groupIndex, int rowIndex,
               boost::optional<Row> const &newValue) {
  if (newValue.is_initialized()) {
    jobs.groups()[groupIndex].updateRow(rowIndex, newValue);
  } else {
    jobs.groups()[groupIndex].updateRow(rowIndex, boost::none);
  }
}

void mergeRowIntoGroup(ReductionJobs &jobs, Row const &row,
                       double thetaTolerance, std::string const &groupName) {
  auto &group = findOrMakeGroupWithName(jobs, groupName);
  auto indexOfRowToUpdate =
      group.indexOfRowWithTheta(row.theta(), thetaTolerance);

  if (indexOfRowToUpdate.is_initialized()) {
    auto newRowValue = mergedRow(group[indexOfRowToUpdate.get()].get(), row);
    group.updateRow(indexOfRowToUpdate.get(), newRowValue);
  } else {
    group.appendRow(row);
  }
}

void removeRow(ReductionJobs &jobs, int groupIndex, int rowIndex) {
  jobs.groups()[groupIndex].removeRow(rowIndex);
}

bool setGroupName(ReductionJobs &jobs, int groupIndex,
                  std::string const &newValue) {
  auto &group = jobs.groups()[groupIndex];
  if (group.name() != newValue) {
    if (newValue.empty() || !jobs.hasGroupWithName(newValue)) {
      group.setName(newValue);
    } else {
      return false;
    }
  }
  return true;
}

std::string groupName(ReductionJobs const &jobs, int groupIndex) {
  return jobs[groupIndex].name();
}

void prettyPrintModel(ReductionJobs const & /*jobs*/) {
  std::cout << "ReductionJobs:\n";
  //  for (auto &&group : jobs.groups()) {
  //    std::cout << group;
  //  }
  std::cout << std::endl;
}

Group const &ReductionJobs::operator[](int index) const {
  return m_groups[index];
}
} // namespace CustomInterfaces
} // namespace MantidQt
