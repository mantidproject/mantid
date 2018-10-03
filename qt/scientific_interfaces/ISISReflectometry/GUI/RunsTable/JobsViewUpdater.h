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
#ifndef MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
#define MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "Map.h"
#include "Reduction/Group.h"

namespace MantidQt {
namespace CustomInterfaces {

namespace { // unnamed
std::vector<MantidQt::MantidWidgets::Batch::Cell>
cellsFromGroup(Group const &group,
               MantidQt::MantidWidgets::Batch::Cell const &deadCell) {
  auto cells = std::vector<MantidQt::MantidWidgets::Batch::Cell>(9, deadCell);
  cells[0] = MantidQt::MantidWidgets::Batch::Cell(group.name());
  return cells;
}

std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromRow(Row const &row) {
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(std::to_string(row.theta())),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().first),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().second),
       MantidQt::MantidWidgets::Batch::Cell(
           optionalToString(row.qRange().min())),
       MantidQt::MantidWidgets::Batch::Cell(
           optionalToString(row.qRange().max())),
       MantidQt::MantidWidgets::Batch::Cell(
           optionalToString(row.qRange().step())),
       MantidQt::MantidWidgets::Batch::Cell(
           optionalToString(row.scaleFactor())),
       MantidQt::MantidWidgets::Batch::Cell(
           MantidWidgets::optionsToString(row.reductionOptions()))});
}
} // namespace

class JobsViewUpdater {
public:
  explicit JobsViewUpdater(MantidQt::MantidWidgets::Batch::IJobTreeView &view)
      : m_view(view) {}

  void groupAppended(int groupIndex, Group const &group) {
    m_view.appendChildRowOf(MantidQt::MantidWidgets::Batch::RowLocation(),
                            cellsFromGroup(group, m_view.deadCell()));
    for (auto const &row : group.rows())
      if (row.is_initialized())
        m_view.appendChildRowOf(
            MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}),
            cellsFromRow(row.get()));
  }

  void rowAppended(int groupIndex, int, Row const &row) {
    m_view.appendChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}),
        cellsFromRow(row));
  }

  void rowModified(int groupIndex, int rowIndex, Row const &row) {
    m_view.setCellsAt(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex, rowIndex}),
        cellsFromRow(row));
  }

private:
  MantidQt::MantidWidgets::Batch::IJobTreeView &m_view;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
