// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "JobsViewUpdater.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

using ValueFunction = std::optional<double> (RangeInQ::*)() const;

namespace { // unnamed
std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromGroup(Group const &group,
                                                                 MantidQt::MantidWidgets::Batch::Cell const &deadCell) {
  auto cells = std::vector<MantidQt::MantidWidgets::Batch::Cell>(9, deadCell);
  cells[0] = MantidQt::MantidWidgets::Batch::Cell(group.name());
  return cells;
}

MantidWidgets::Batch::Cell qRangeCellOrDefault(RangeInQ const &qRangeInput, RangeInQ const &qRangeOutput,
                                               ValueFunction valueFunction, std::optional<int> precision) {
  auto maybeValue = (qRangeInput.*valueFunction)();
  auto useOutputValue = false;
  if (!maybeValue.has_value()) {
    maybeValue = (qRangeOutput.*valueFunction)();
    useOutputValue = true;
  }
  auto result = MantidWidgets::Batch::Cell(optionalToString(maybeValue, precision));
  if (useOutputValue)
    result.setOutput();
  else
    result.setInput();
  return result;
}

boost::optional<size_t> incrementIndex(const Row &row) {
  auto lookupIndex = row.lookupIndex();
  return lookupIndex.is_initialized() ? boost::optional<size_t>(lookupIndex.get() + 1) : boost::none;
}

std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromRow(Row const &row, boost::optional<int> precision) {
  // convert type for precision
  std::optional<int> precisionStd = std::nullopt;
  if (precision)
    precisionStd = precision.get();

  auto lookupIndex = incrementIndex(row);
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(valueToString(row.theta(), precision)),
       MantidQt::MantidWidgets::Batch::Cell(row.transmissionWorkspaceNames().firstRunList()),
       MantidQt::MantidWidgets::Batch::Cell(row.transmissionWorkspaceNames().secondRunList()),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::min, precisionStd),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::max, precisionStd),
       qRangeCellOrDefault(row.qRange(), row.qRangeOutput(), &RangeInQ::step, precisionStd),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(row.scaleFactor(), precision)),
       MantidQt::MantidWidgets::Batch::Cell(MantidWidgets::optionsToString(row.reductionOptions())),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(lookupIndex, precision))});
}
} // namespace

void JobsViewUpdater::groupAppended(int groupIndex, Group const &group) {
  m_view.appendChildRowOf(MantidQt::MantidWidgets::Batch::RowLocation(), cellsFromGroup(group, m_view.deadCell()));
  for (auto const &row : group.rows())
    if (row.is_initialized())
      m_view.appendChildRowOf(MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}),
                              cellsFromRow(row.get(), m_precision));
}

void JobsViewUpdater::groupRemoved(int groupIndex) {
  m_view.removeRowAt(MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}));
}

void JobsViewUpdater::rowInserted(int groupIndex, int rowIndex, Row const &row) {
  m_view.insertChildRowOf(MantidQt::MantidWidgets::Batch::RowLocation({groupIndex}), rowIndex,
                          cellsFromRow(row, m_precision));
}

void JobsViewUpdater::rowModified(int groupIndex, int rowIndex, Row const &row) {
  m_view.setCellsAt(MantidQt::MantidWidgets::Batch::RowLocation({groupIndex, rowIndex}),
                    cellsFromRow(row, m_precision));
}

void JobsViewUpdater::setPrecision(const int &precision) { m_precision = precision; }

void JobsViewUpdater::resetPrecision() { m_precision = boost::none; }

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
