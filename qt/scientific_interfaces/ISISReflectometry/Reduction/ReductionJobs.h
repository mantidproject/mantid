/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#define MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
#include "../DllConfig.h"
#include <boost/optional.hpp>
#include "../multivisitors.hpp"
// equivalent to
//         #include <boost/variant/multivisitors.hpp>
// available in boost 1.54+ - required for RHEL7.

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

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL ReductionJobs<SlicedGroup>;
using SlicedReductionJobs = ReductionJobs<SlicedGroup>;

extern template class MANTIDQT_ISISREFLECTOMETRY_DLL
    ReductionJobs<UnslicedGroup>;
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
class MergeJobsMultivisitor : public boost::static_visitor<bool> {
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

Jobs newJobsWithSlicingFrom(Jobs const &takeSlicingFromHere);

UnslicedReductionJobs
unsliced(SlicedReductionJobs const &slicedJobs,
         WorkspaceNamesFactory const &workspaceNamesFactory);

SlicedReductionJobs sliced(UnslicedReductionJobs const &unslicedJobs,
                           WorkspaceNamesFactory const &workspaceNamesFactory);
}
}

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
