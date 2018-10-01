#include "ReductionJobs.h"
#include "../IndexOf.h"
#include "../Map.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

namespace { // unnamed
std::string emptyGroupName() {
  static int nameSuffix = 1;
  std::string name = "Group" + std::to_string(nameSuffix++);
  return name;
}

Group &findOrMakeGroupWithName(Jobs &jobs, std::string const &groupName) {
  auto maybeGroupIndex = jobs.indexOfGroupWithName(groupName);
  if (maybeGroupIndex.is_initialized())
    return jobs.groups()[maybeGroupIndex.get()];
  else
    return jobs.appendGroup(Group(groupName));
} // unnamed
} // namespace

Jobs::Jobs(std::vector<Group> groups) : m_groups(std::move(groups)) {}

Jobs::Jobs() {}

Group &Jobs::appendGroup(Group group) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  m_groups.emplace_back(std::move(group));
  return m_groups.back();
}

boost::optional<int> Jobs::indexOfGroupWithName(std::string const &groupName) {
  return indexOf(m_groups, [&groupName](Group const &group) -> bool {
    return group.name() == groupName;
  });
}

Group &Jobs::insertGroup(Group group, int beforeIndex) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  return *m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
}

bool Jobs::hasGroupWithName(std::string const &groupName) const {
  return std::any_of(m_groups.crbegin(), m_groups.crend(),
                     [&groupName](Group const &group) -> bool {
                       return group.name() == groupName;
                     });
}

void Jobs::removeGroup(int index) { m_groups.erase(m_groups.begin() + index); }

std::vector<Group> &Jobs::groups() { return m_groups; }

std::vector<Group> const &Jobs::groups() const { return m_groups; }

void removeGroup(Jobs &jobs, int groupIndex) { jobs.removeGroup(groupIndex); }

void appendEmptyRow(Jobs &jobs, int groupIndex) {
  jobs.groups()[groupIndex].appendEmptyRow();
}

void appendEmptyGroup(Jobs &jobs) { jobs.appendGroup(Group(emptyGroupName())); }

void insertEmptyGroup(Jobs &jobs, int beforeGroup) {
  jobs.insertGroup(Group(emptyGroupName()), beforeGroup);
}

void insertEmptyRow(Jobs &jobs, int groupIndex, int beforeRow) {
  jobs.groups()[groupIndex].insertRow(boost::none, beforeRow);
}

void updateRow(Jobs &jobs, int groupIndex, int rowIndex,
               boost::optional<Row> const &newValue) {
  if (newValue.is_initialized()) {
    jobs.groups()[groupIndex].updateRow(rowIndex, newValue);
  } else {
    jobs.groups()[groupIndex].updateRow(rowIndex, boost::none);
  }
}

void mergeRowIntoGroup(Jobs &jobs, Row const &row, double thetaTolerance,
                       std::string const &groupName) {
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

void removeRow(Jobs &jobs, int groupIndex, int rowIndex) {
  jobs.groups()[groupIndex].removeRow(rowIndex);
}

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue) {
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

std::string groupName(Jobs const &jobs, int groupIndex) {
  return jobs[groupIndex].name();
}

void prettyPrintModel(Jobs const & /*jobs*/) {
  std::cout << "Jobs:\n";
  //  for (auto &&group : jobs.groups()) {
  //    std::cout << group;
  //  }
  std::cout << std::endl;
}

Group const &Jobs::operator[](int index) const { return m_groups[index]; }
} // namespace CustomInterfaces
} // namespace MantidQt
