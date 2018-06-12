#include "ReductionJobs.h"
#include <iostream>
namespace MantidQt {
namespace CustomInterfaces {

template <typename Group>
ReductionJobs<Group>::ReductionJobs(std::vector<Group> groups)
    : m_groups(std::move(groups)) {}

template <typename Group>
Group &ReductionJobs<Group>::appendGroup(Group group) {
  assert(!hasGroupWithName(group.name()));
  m_groups.emplace_back(std::move(group));
  return m_groups.back();
}

template <typename Group>
Group &ReductionJobs<Group>::insertGroup(Group group, int beforeIndex) {
  assert(!hasGroupWithName(group.name()));
  return *m_groups.insert(m_groups.begin() + beforeIndex, std::move(group));
}

template <typename Group>
bool ReductionJobs<Group>::hasGroupWithName(
    std::string const &groupName) const {
  return std::find_if(m_groups.crbegin(), m_groups.crend(),
                      [&groupName](Group const &group) -> bool {
                        return group.name() == groupName;
                      }) == m_groups.crend();
}

template <typename Group> void ReductionJobs<Group>::removeGroup(int index) {
  m_groups.erase(m_groups.cbegin() + index);
}

template <typename Group> std::vector<Group> &ReductionJobs<Group>::groups() {
  return m_groups;
}

template <typename Group>
std::vector<Group> const &ReductionJobs<Group>::groups() const {
  return m_groups;
}

class RemoveGroupVisitor : boost::static_visitor<> {
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

class AppendEmptyRowVisitor : boost::static_visitor<> {
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

class AppendEmptyGroupVisitor : boost::static_visitor<> {
public:
  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    jobs.appendGroup(Group(""));
  }
};

void appendEmptyGroup(Jobs &jobs) {
  boost::apply_visitor(AppendEmptyGroupVisitor(), jobs);
}

class InsertEmptyGroupVisitor : boost::static_visitor<> {
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

class InsertEmptyRowVisitor : boost::static_visitor<> {
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

class UpdateRowVisitor : boost::static_visitor<> {
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
class MergeRowIntoGroupVisitor : boost::static_visitor<> {
public:
  MergeRowIntoGroupVisitor(RowVariant const &row, double thetaTolerance,
                           std::string const &groupName,
                           WorkspaceNamesFactory const &workspaceNames)
      : m_row(row), m_thetaTolerance(thetaTolerance), m_groupName(groupName),
        m_workspaceNames(workspaceNames) {}

  template <typename Group>
  Group &findOrMakeGroupWithName(ReductionJobs<Group> &jobs,
                                 std::string const &groupName) {
    if (jobs.hasGroupWithName(groupName))
      return jobs.appendGroup(Group(groupName));
    else
      return jobs.findGroupByName(groupName);
  }

  template <typename Group> void operator()(ReductionJobs<Group> &jobs) const {
    auto &group = findOrMakeGroupWithName(m_groupName);

    auto const &row = boost::get<typename Group::Row>(m_row);
    auto findRowResult = group.findRowWithTheta(row.theta(), m_thetaTolerance);
    auto existRowIndex = findRowResult.first;
    auto *existingRowPtr = findRowResult.second;
    if (existingRowPtr == nullptr) {
      group.appendRow(m_row);
    } else {
      auto newRowValue = existingRowPtr->withExtraRunNumbers(row.runNumbers(),
                                                             m_workspaceNames);
      group.updateRow(existRowIndex, newRowValue);
    }
  }

private:
  RowVariant const &m_row;
  double m_thetaTolerance;
  std::string const &m_groupName;
  WorkspaceNamesFactory const &m_workspaceNames;
};

template <typename WorkspaceNameFactory>
void mergeRowIntoGroup(Jobs &jobs, RowVariant const &row, double thetaTolerance,
                       std::string const &groupName,
                       WorkspaceNameFactory workspaceNames) {
  boost::apply_visitor(MergeRowIntoGroupVisitor<WorkspaceNameFactory>(
                           row, thetaTolerance, groupName, workspaceNames),
                       jobs);
}

class RemoveRowVisitor : boost::static_visitor<> {
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

class SetGroupNameVisitor : boost::static_visitor<bool> {
public:
  SetGroupNameVisitor(int groupIndex, std::string const &newName)
      : m_groupIndex(groupIndex), m_newName(newName) {}

  template <typename T> bool operator()(ReductionJobs<T> &jobs) const {
    jobs.groups()[m_groupIndex].setName(m_newName);
    return true;
  }

private:
  int m_groupIndex;
  std::string m_newName;
};

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue) {
  return boost::apply_visitor(SetGroupNameVisitor(groupIndex, newValue), jobs);
}

class PrettyPrintVisitor : boost::static_visitor<> {
public:
  template <typename T> void operator()(ReductionJobs<T> const &jobs) const {
    for (auto &&group : jobs.groups()) {
      std::cout << "Group (" << group.name() << ")\n";
      for (auto &&row : group.rows()) {
        if (row.is_initialized()) {
          if (row.get().runNumbers().empty())
            std::cout << "  Row (empty)\n";
          else
            std::cout << "  Row (run number: " << row.get().runNumbers()[0]
                      << ")\n";
        } else {
          std::cout << "  Row (invalid)\n";
        }
      }
    }
    std::cout << std::endl;
  }
};

void prettyPrintModel(Jobs const &jobs) {
  boost::apply_visitor(PrettyPrintVisitor(), jobs);
}

SlicedReductionJobs sliced(UnslicedReductionJobs const &unslicedJobs) {
  auto slicedGroups = std::vector<SlicedGroup>();
  auto const &unslicedGroups = unslicedJobs.groups();
  std::transform(unslicedGroups.begin(), unslicedGroups.end(),
                 std::back_inserter(slicedGroups),
                 static_cast<SlicedGroup (*)(UnslicedGroup const &)>(&slice));
  return SlicedReductionJobs(std::move(slicedGroups));
}

UnslicedReductionJobs unsliced(SlicedReductionJobs const &slicedJobs) {
  auto unslicedGroups = std::vector<UnslicedGroup>();
  auto const &slicedGroups = slicedJobs.groups();
  std::transform(slicedGroups.begin(), slicedGroups.end(),
                 std::back_inserter(unslicedGroups),
                 static_cast<UnslicedGroup (*)(SlicedGroup const &)>(&unslice));
  return UnslicedReductionJobs(std::move(unslicedGroups));
}

template <typename Group>
Group const &ReductionJobs<Group>::operator[](int index) const {
  return m_groups[index];
}
}
}
