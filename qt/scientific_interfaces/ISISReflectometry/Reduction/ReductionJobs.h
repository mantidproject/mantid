#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>
#include <boost/variant/multivisitors.hpp>
#include "Group.h"
#include "WorkspaceNamesFactory.h"

namespace MantidQt {
namespace CustomInterfaces {

template <typename Group> class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs {
public:
  ReductionJobs();
  ReductionJobs(std::vector<Group> groups);
  Group &appendGroup(Group group);
  Group &insertGroup(Group group, int beforeIndex);
  bool hasGroupWithName(std::string const &groupName) const;
  boost::optional<int> indexOfGroupWithName(std::string const &groupName);
  void removeGroup(int index);

  std::vector<Group> &groups();
  std::vector<Group> const &groups() const;
  Group const &operator[](int index) const;

private:
  std::vector<Group> m_groups;
};

template class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs<SlicedGroup>;
using SlicedReductionJobs = ReductionJobs<SlicedGroup>;

template class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs<UnslicedGroup>;
using UnslicedReductionJobs = ReductionJobs<UnslicedGroup>;

using Jobs = boost::variant<UnslicedReductionJobs, SlicedReductionJobs>;

void appendEmptyRow(Jobs &jobs, int groupIndex);
void insertEmptyRow(Jobs &jobs, int groupIndex, int beforeRow);
void removeRow(Jobs &jobs, int groupIndex, int rowIndex);
void updateRow(Jobs &jobs, int groupIndex, int rowIndex,
               boost::optional<RowVariant> const &newValue);

void appendEmptyGroup(Jobs &jobs);
void insertEmptyGroup(Jobs &jobs, int beforeGroup);
void removeGroup(Jobs &jobs, int groupIndex);

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue);
std::string groupName(Jobs const &jobs, int groupIndex);
void prettyPrintModel(Jobs const &jobs);

void mergeRowIntoGroup(Jobs &jobs, RowVariant const &row, double thetaTolerance,
                       std::string const &groupName,
                       WorkspaceNamesFactory const &workspaceNamesFactory);

template <typename WorkspaceNames, typename WorkspaceNamesFactory>
Row<WorkspaceNames>
mergedRow(Row<WorkspaceNames> const &rowA, Row<WorkspaceNames> const &rowB,
          WorkspaceNamesFactory const &workspaceNamesFactory) {
  return rowA.withExtraRunNumbers(rowB.runNumbers(), workspaceNamesFactory);
}

template <typename Row, typename WorkspaceNamesFactory,
          typename ModificationListener>
void mergeRowsInto(Group<Row> &intoHere, Group<Row> const &fromHere,
                   int groupIndex, double thetaTolerance,
                   WorkspaceNamesFactory const &workspaceNamesFactory,
                   ModificationListener &listener) {
  for (auto const &maybeRow : fromHere.rows()) {
    if (maybeRow.is_initialized()) {
      auto const &fromRow = maybeRow.get();
      auto index =
          intoHere.indexOfRowWithTheta(fromRow.theta(), thetaTolerance);
      if (index.is_initialized()) {
        auto const updateAtIndex = index.get();
        auto const &intoRow = intoHere[updateAtIndex].get();
        auto updatedRow = mergedRow(intoRow, fromRow, workspaceNamesFactory);
        intoHere.updateRow(updateAtIndex, updatedRow);
        listener.rowModified(groupIndex, updateAtIndex, updatedRow);
      } else {
        intoHere.appendRow(maybeRow.get());
        listener.rowAppended(groupIndex,
                             static_cast<int>(intoHere.rows().size() - 1),
                             maybeRow.get());
      }
    }
  }
}

template <typename Group, typename WorkspaceNamesFactory,
          typename ModificationListener>
void mergeJobsInto(ReductionJobs<Group> &intoHere,
                   ReductionJobs<Group> const &fromHere, double thetaTolerance,
                   WorkspaceNamesFactory const &workspaceNamesFactory,
                   ModificationListener &listener) {
  for (auto const &group : fromHere.groups()) {
    auto maybeGroupIndex = intoHere.indexOfGroupWithName(group.name());
    if (maybeGroupIndex.is_initialized()) {
      auto indexToUpdateAt = maybeGroupIndex.get();
      auto &intoGroup = intoHere.groups()[indexToUpdateAt];
      mergeRowsInto(intoGroup, group, indexToUpdateAt, thetaTolerance,
                    workspaceNamesFactory, listener);
    } else {
      intoHere.appendGroup(group);
      listener.groupAppended(static_cast<int>(intoHere.groups().size()) - 1,
                             group);
    }
  }
}

template <typename WorkspaceNamesFactory, typename ModificationListener>
class MergeJobsMultivisitor : boost::static_visitor<bool> {
public:
  MergeJobsMultivisitor(double thetaTolerance,
                        WorkspaceNamesFactory const &workspaceNamesFactory,
                        ModificationListener &listener)
      : m_thetaTolerance(thetaTolerance),
        m_workspaceNamesFactory(workspaceNamesFactory), m_listener(listener){};

  template <typename Group>
  bool operator()(ReductionJobs<Group> &intoHere,
                  ReductionJobs<Group> const &fromHere) const {
    mergeJobsInto(intoHere, fromHere, m_thetaTolerance, m_workspaceNamesFactory,
                  m_listener);
    return true;
  }

  template <typename ErrorGroupA, typename ErrorGroupB>
  bool operator()(ReductionJobs<ErrorGroupA> &,
                  ReductionJobs<ErrorGroupB> const &) const {
    return false;
  }

private:
  double m_thetaTolerance;
  WorkspaceNamesFactory const &m_workspaceNamesFactory;
  ModificationListener &m_listener;
};

template <typename WorkspaceNamesFactory, typename ModificationListener>
bool mergeJobsInto(Jobs &intoHere, Jobs const &fromHere, double thetaTolerance,
                   WorkspaceNamesFactory const &workspaceNamesFactory,
                   ModificationListener &listener) {
  return boost::apply_visitor(
      MergeJobsMultivisitor<WorkspaceNamesFactory, ModificationListener>(
          thetaTolerance, workspaceNamesFactory, listener),
      intoHere, fromHere);
}

Jobs newJobsWithSlicingFrom(Jobs const& jobs);

UnslicedReductionJobs
unsliced(SlicedReductionJobs const &slicedJobs,
         WorkspaceNamesFactory const &workspaceNamesFactory);
SlicedReductionJobs sliced(UnslicedReductionJobs const &unslicedJobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory);
}
}

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
