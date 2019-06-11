// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
#define MANTID_ISISREFLECTOMETRY_JOBVIEWUPDATER_H
#include "Common/Map.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
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

using ValueFunction = boost::optional<double> (RangeInQ::*)() const;

MantidWidgets::Batch::Cell qRangeCellOrDefault(RangeInQ const &qRangeInput,
                                               RangeInQ const &qRangeOutput,
                                               ValueFunction valueFunction) {
  auto maybeValue = (qRangeInput.*valueFunction)();
  auto useOutputValue = false;
  if (!maybeValue.is_initialized()) {
    maybeValue = (qRangeOutput.*valueFunction)();
    useOutputValue = true;
  }
  auto result = MantidWidgets::Batch::Cell(optionalToString(maybeValue));
  if (useOutputValue)
    result.setOutput();
  else
    result.setInput();
  return result;
}

std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromRow(Row const &row) {
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(std::to_string(row.theta())),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().firstRunList()),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().secondRunList()),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::min),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::max),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::step),
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

  void groupRemoved(int groupIndex) {
    m_view.removeRowAt(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}));
  }

  void rowInserted(int groupIndex, int rowIndex, Row const &row) {
    m_view.insertChildRowOf(
        MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}), rowIndex,
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
