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
#ifndef MANTID_CUSTOMINTERFACES_GROUP_H_
#define MANTID_CUSTOMINTERFACES_GROUP_H_
#include "../DllConfig.h"
#include "Row.h"
#include "WorkspaceNamesFactory.h"
#include <boost/optional.hpp>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

template <typename Row> class MANTIDQT_ISISREFLECTOMETRY_DLL Group {
public:
  using RowType = Row;
  Group(std::string name);
  Group(std::string name, std::vector<boost::optional<Row>> rows);

  std::string const &name() const;
  void setName(std::string const &name);
  std::string postprocessedWorkspaceName(
      WorkspaceNamesFactory const &workspaceNamesFactory) const;

  void appendEmptyRow();
  void appendRow(boost::optional<Row> const &row);
  void insertRow(boost::optional<Row> const &row, int beforeRowAtIndex);
  void removeRow(int rowIndex);
  void updateRow(int rowIndex, boost::optional<Row> const &row);
  bool allRowsAreValid() const;

  boost::optional<int> indexOfRowWithTheta(double angle,
                                           double tolerance) const;

  boost::optional<Row> const &operator[](int rowIndex) const;
  std::vector<boost::optional<Row>> const &rows() const;

private:
  std::string m_name;
  std::vector<boost::optional<Row>> m_rows;
};

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

template <typename Row>
std::ostream &operator<<(std::ostream &os, Group<Row> const &group) {
  os << "  Group (name: " << group.name() << ")\n";
  for (auto &&row : group.rows()) {
    if (row.is_initialized())
      os << "    " << row.get() << '\n';
    else
      os << "    Row (invalid)\n";
  }
  return os;
}

using SlicedGroup = Group<SlicedRow>;
using UnslicedGroup = Group<UnslicedRow>;

UnslicedGroup unslice(SlicedGroup const &slicedGroup,
                      WorkspaceNamesFactory const &workspaceNamesFactory);
SlicedGroup slice(UnslicedGroup const &unslicedGroup,
                  WorkspaceNamesFactory const &WorkspaceNamesFactory);
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_GROUP_H_
