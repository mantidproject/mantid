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
namespace MantidQt {
namespace CustomInterfaces {

class BatchViewJobsUpdater {
public:
  explicit BatchViewJobsUpdater(
      MantidQt::MantidWidgets::Batch::IJobTreeView &view)
      : m_view(view) {}

  template <typename Group>
  void groupAppended(int groupIndex, Group const &group) {
    m_view.appendChildRowOf(MantidQt::MantidWidgets::Batch::RowLocation(),
                            cellsFromGroup(group, m_view.deadCell()));
    for (auto const &row : group.rows())
      if (row.is_initialized())
        m_view.appendChildRowOf(
            MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}),
            cellsFromRow(row.get()));
  }

  template <typename Row>
  void rowAppended(int groupIndex, int, Row const &row) {
    m_view.appendChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}),
        cellsFromRow(row));
  }

  template <typename Row>
  void rowModified(int groupIndex, int rowIndex, Row const &row) {
    m_view.setCellsAt(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex, rowIndex}),
        cellsFromRow(row));
  }

private:
  MantidQt::MantidWidgets::Batch::IJobTreeView &m_view;
};
}
}
#endif // MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
