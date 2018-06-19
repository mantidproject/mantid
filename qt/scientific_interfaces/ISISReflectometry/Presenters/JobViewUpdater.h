#ifndef MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
#define MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
namespace MantidQt {
namespace CustomInterfaces {

class BatchViewJobsUpdater {
public:
  explicit BatchViewJobsUpdater(MantidQt::MantidWidgets::Batch::IJobTreeView &view)
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
