#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "ReductionJobs.h"
#include "../Map.h"
#include "../IndexOf.h"
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

template <typename Group>
ReductionJobs<Group>::ReductionJobs(std::vector<Group> groups)
    : m_groups(std::move(groups)) {}

template <typename Group> ReductionJobs<Group>::ReductionJobs() {}

// template <typename Group>
// WorkspaceNamesFactory const *
// ReductionJobs<Group>::workspaceNamesFactory() const {
//  return m_workspaceNamesFactory;
//}

template <typename Group>
Group &ReductionJobs<Group>::appendGroup(Group group) {
  assertOrThrow(group.name().empty() || !hasGroupWithName(group.name()),
                "Cannot have multiple groups with a matching non-empty name.");
  m_groups.emplace_back(std::move(group));
  return m_groups.back();
}

template <typename Group>
boost::optional<int>
ReductionJobs<Group>::indexOfGroupWithName(std::string const &groupName) {
  return indexOf(m_groups, [&groupName](Group const &group)
                               -> bool { return group.name() == groupName; });
}

template <typename Group>
Group &ReductionJobs<Group>::insertGroup(Group group, int beforeIndex) {
  assert(!hasGroupWithName(group.name()));
  return *m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
}

template <typename Group>
bool ReductionJobs<Group>::hasGroupWithName(
    std::string const &groupName) const {
  return std::any_of(m_groups.crbegin(), m_groups.crend(),
                     [&groupName](Group const &group)
                         -> bool { return group.name() == groupName; });
}

template <typename Group> void ReductionJobs<Group>::removeGroup(int index) {
  m_groups.erase(m_groups.begin() + index);
}

template <typename Group> std::vector<Group> &ReductionJobs<Group>::groups() {
  return m_groups;
}

template <typename Group>
std::vector<Group> const &ReductionJobs<Group>::groups() const {
  return m_groups;
}

class RemoveGroupVisitor : public boost::static_visitor<> {
public:
  RemoveGroupVisitor(int groupIndex) : m_groupIndex(groupIndex) {}

  template <typename T> void operator()(ReductionJobs<T> &jobs) const {
    jobs.removeGroup(m_groupIndex);
  }

private:
  int m_groupIndex;
};

void removeGroup(Jobs &jobs, int groupIndex) {
  boost::apply_visitor(RemoveGroupVisitor(groupIndex), jobs);
}

class AppendEmptyRowVisitor : public boost::static_visitor<> {
public:
  AppendEmptyRowVisitor(int groupIndex) : m_groupIndex(groupIndex) {}

  template <typename T> void operator()(ReductionJobs<T> &jobs) const {
    jobs.groups()[m_groupIndex].appendEmptyRow();
  }

private:
  int m_groupIndex;
};

void appendEmptyRow(Jobs &jobs, int groupIndex) {
  boost::apply_visitor(AppendEmptyRowVisitor(groupIndex), jobs);
}

class AppendEmptyGroupVisitor : public boost::static_visitor<> {
public:
  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    jobs.appendGroup(Group(""));
  }
};

void appendEmptyGroup(Jobs &jobs) {
  boost::apply_visitor(AppendEmptyGroupVisitor(), jobs);
}

class InsertEmptyGroupVisitor : public boost::static_visitor<> {
public:
  InsertEmptyGroupVisitor(int beforeGroup) : m_beforeGroup(beforeGroup) {}

  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    jobs.insertGroup(Group(""), m_beforeGroup);
  }

private:
  int m_beforeGroup;
};

void insertEmptyGroup(Jobs &jobs, int beforeGroup) {
  boost::apply_visitor(InsertEmptyGroupVisitor(beforeGroup), jobs);
}

class InsertEmptyRowVisitor : public boost::static_visitor<> {
public:
  InsertEmptyRowVisitor(int groupIndex, int beforeRow)
      : m_groupIndex(groupIndex), m_beforeRow(beforeRow) {}

  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    jobs.groups()[m_groupIndex].insertRow(boost::none, m_beforeRow);
  }

private:
  int m_groupIndex;
  int m_beforeRow;
};

void insertEmptyRow(Jobs &jobs, int groupIndex, int beforeRow) {
  boost::apply_visitor(InsertEmptyRowVisitor(groupIndex, beforeRow), jobs);
}

class UpdateRowVisitor : public boost::static_visitor<> {
public:
  UpdateRowVisitor(int groupIndex, int rowIndex,
                   boost::optional<RowVariant> const &row)
      : m_groupIndex(groupIndex), m_rowIndex(rowIndex), m_row(row) {}

  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    if (m_row.is_initialized()) {
      jobs.groups()[m_groupIndex].updateRow(
          m_rowIndex, boost::get<typename Group::RowType>(m_row.get()));
    } else {
      jobs.groups()[m_groupIndex].updateRow(m_rowIndex, boost::none);
    }
  }

private:
  int m_groupIndex;
  int m_rowIndex;
  boost::optional<RowVariant> const &m_row;
};

void updateRow(Jobs &jobs, int groupIndex, int rowIndex,
               boost::optional<RowVariant> const &newValue) {
  boost::apply_visitor(UpdateRowVisitor(groupIndex, rowIndex, newValue), jobs);
}

template <typename WorkspaceNamesFactory>
class MergeRowIntoGroupVisitor : public boost::static_visitor<> {
public:
  MergeRowIntoGroupVisitor(RowVariant const &row, double thetaTolerance,
                           std::string const &groupName,
                           WorkspaceNamesFactory const &workspaceNames)
      : m_row(row), m_thetaTolerance(thetaTolerance), m_groupName(groupName),
        m_workspaceNames(workspaceNames) {}

  template <typename Group>
  Group &findOrMakeGroupWithName(ReductionJobs<Group> &jobs,
                                 std::string const &groupName) const {
    auto maybeGroupIndex = jobs.indexOfGroupWithName(groupName);
    if (maybeGroupIndex.is_initialized())
      return jobs.groups()[maybeGroupIndex.get()];
    else
      return jobs.appendGroup(Group(groupName));
  }

  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    auto &group = findOrMakeGroupWithName(jobs, m_groupName);
    auto const &row = boost::get<typename Group::RowType>(m_row);
    auto indexOfRowToUpdate =
        group.indexOfRowWithTheta(row.theta(), m_thetaTolerance);

    if (indexOfRowToUpdate.is_initialized()) {
      auto newRowValue = mergedRow(group[indexOfRowToUpdate.get()].get(), row,
                                   m_workspaceNames);
      group.updateRow(indexOfRowToUpdate.get(), newRowValue);
    } else {
      group.appendRow(row);
    }
  }

private:
  RowVariant const &m_row;
  double m_thetaTolerance;
  std::string const &m_groupName;
  WorkspaceNamesFactory const &m_workspaceNames;
};

void mergeRowIntoGroup(Jobs &jobs, RowVariant const &row, double thetaTolerance,
                       std::string const &groupName,
                       WorkspaceNamesFactory const &workspaceNamesFactory) {
  boost::apply_visitor(
      MergeRowIntoGroupVisitor<WorkspaceNamesFactory>(
          row, thetaTolerance, groupName, workspaceNamesFactory),
      jobs);
}

class RemoveRowVisitor : public boost::static_visitor<> {
public:
  RemoveRowVisitor(int groupIndex, int rowIndex)
      : m_groupIndex(groupIndex), m_rowIndex(rowIndex) {}

  template <typename T> void operator()(ReductionJobs<T> &jobs) const {
    jobs.groups()[m_groupIndex].removeRow(m_rowIndex);
  }

private:
  int m_groupIndex;
  int m_rowIndex;
};

void removeRow(Jobs &jobs, int groupIndex, int rowIndex) {
  boost::apply_visitor(RemoveRowVisitor(groupIndex, rowIndex), jobs);
}

class SetGroupNameVisitor : public boost::static_visitor<bool> {
public:
  SetGroupNameVisitor(int groupIndex, std::string const &newName)
      : m_groupIndex(groupIndex), m_newName(newName) {}

  template <typename T> bool operator()(ReductionJobs<T> &jobs) const {
    auto &group = jobs.groups()[m_groupIndex];
    if (group.name() != m_newName) {
      if (m_newName.empty() || !jobs.hasGroupWithName(m_newName)) {
        group.setName(m_newName);
      } else {
        return false;
      }
    }
    return true;
  }

private:
  int m_groupIndex;
  std::string m_newName;
};

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue) {
  return boost::apply_visitor(SetGroupNameVisitor(groupIndex, newValue), jobs);
}

class GroupNameVisitor : public boost::static_visitor<std::string> {
public:
  GroupNameVisitor(int groupIndex) : m_groupIndex(groupIndex) {}

  template <typename T>
  std::string operator()(ReductionJobs<T> const &jobs) const {
    return jobs[m_groupIndex].name();
  }

private:
  int m_groupIndex;
};

std::string groupName(Jobs const &jobs, int groupIndex) {
  return boost::apply_visitor(GroupNameVisitor(groupIndex), jobs);
}

class PrettyPrintVisitor : public boost::static_visitor<> {
public:
  void operator()(SlicedReductionJobs const &jobs) const {
    std::cout << "Sliced Jobs:";
    for (auto &&group : jobs.groups()) {
      std::cout << group;
    }
    std::cout << std::endl;
  }

  void operator()(UnslicedReductionJobs const &jobs) const {
    std::cout << "Unsliced Jobs:\n";
    for (auto &&group : jobs.groups()) {
      std::cout << group;
    }
    std::cout << std::endl;
  }
};

void prettyPrintModel(Jobs const &jobs) {
  boost::apply_visitor(PrettyPrintVisitor(), jobs);
}

class NewJobsWithSlicingFrom : public boost::static_visitor<Jobs> {
public:
  Jobs operator()(SlicedReductionJobs const &) const {
    return SlicedReductionJobs();
  }

  Jobs operator()(UnslicedReductionJobs const &) const {
    return UnslicedReductionJobs();
  }
};

Jobs newJobsWithSlicingFrom(Jobs const &jobs) {
  return boost::apply_visitor(NewJobsWithSlicingFrom(), jobs);
}

SlicedReductionJobs sliced(UnslicedReductionJobs const &unslicedJobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory) {
  auto const &unslicedGroups = unslicedJobs.groups();
  auto slicedGroups =
      map(unslicedGroups, [&](UnslicedGroup const &unsliced) -> SlicedGroup {
        return slice(unsliced, workspaceNamesFactory);
      });
  return SlicedReductionJobs(std::move(slicedGroups));
}

UnslicedReductionJobs
unsliced(SlicedReductionJobs const &slicedJobs,
         WorkspaceNamesFactory const &workspaceNamesFactory) {
  auto const &slicedGroups = slicedJobs.groups();
  auto unslicedGroups =
      map(slicedGroups, [&](SlicedGroup const &sliced) -> UnslicedGroup {
        return unslice(sliced, workspaceNamesFactory);
      });
  return UnslicedReductionJobs(std::move(unslicedGroups));
}

template <typename Group>
Group const &ReductionJobs<Group>::operator[](int index) const {
  return m_groups[index];
}

template class ReductionJobs<SlicedGroup>;
template class ReductionJobs<UnslicedGroup>;
}
}
