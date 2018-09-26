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

#include "Group.h"
#include "WorkspaceNamesFactory.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL Jobs {
public:
  Jobs();
  Jobs(std::vector<Group> groups);
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

void appendEmptyRow(Jobs &jobs, int groupIndex);
void insertEmptyRow(Jobs &jobs, int groupIndex, int beforeRow);
void removeRow(Jobs &jobs, int groupIndex, int rowIndex);
void updateRow(Jobs &jobs, int groupIndex, int rowIndex,
               boost::optional<Row> const &newValue);

void appendEmptyGroup(Jobs &jobs);
void insertEmptyGroup(Jobs &jobs, int beforeGroup);
void removeGroup(Jobs &jobs, int groupIndex);

bool setGroupName(Jobs &jobs, int groupIndex, std::string const &newValue);
std::string groupName(Jobs const &jobs, int groupIndex);
void prettyPrintModel(Jobs const &jobs);

void mergeRowIntoGroup(Jobs &jobs, Row const &row, double thetaTolerance,
                       std::string const &groupName,
                       WorkspaceNamesFactory const &workspaceNamesFactory);

template <typename WorkspaceNamesFactory, typename ModificationListener>
void mergeJobsInto(Jobs &intoHere, Jobs const &fromHere, double thetaTolerance,
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
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTID_CUSTOMINTERFACES_REDUCTIONJOBS_H_
