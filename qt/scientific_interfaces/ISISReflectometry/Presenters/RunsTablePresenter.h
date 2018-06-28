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
#ifndef MANTID_CUSTOMINTERFACES_RUNSTABLEPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_RUNSTABLEPRESENTER_H_
#include "DllConfig.h"
#include <memory>
#include "Views/IRunsTableView.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "Reduction/Group.h"
#include "Reduction/ReductionJobs.h"
#include "JobsViewUpdater.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "Map.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTablePresenter
    : public RunsTableViewSubscriber {
public:
  RunsTablePresenter(IRunsTableView *view, std::vector<std::string> const &instruments,
                 double thetaTolerance,
                 WorkspaceNamesFactory workspaceNamesFactory,
                 Jobs reductionJobs);

  void notifyProcessRequested() override;
  void notifyPauseRequested() override;
  void notifyExpandAllRequested() override;
  void notifyCollapseAllRequested() override;
  void notifyCellTextChanged(
      MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
      std::string const &oldValue, std::string const &newValue) override;
  void notifyRowInserted(MantidQt::MantidWidgets::Batch::RowLocation const &
                             newRowLocation) override;
  void notifyRemoveRowsRequested(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          locationsOfRowsToRemove) override;
  void notifyCopyRowsRequested() override;
  void notifyPasteRowsRequested() override;
  void notifyCutRowsRequested() override;
  void notifyFilterReset() override;
  void notifyFilterChanged(std::string const &filterValue) override;

  void notifyInsertRowRequested() override;
  void notifyInsertGroupRequested() override;
  void notifyDeleteRowRequested() override;
  void notifyDeleteGroupRequested() override;

  void mergeAdditionalJobs(Jobs const &jobs);
  Jobs const &reductionJobs() const;

private:
  void
  applyGroupStylingToRow(MantidWidgets::Batch::RowLocation const &location);
  void clearInvalidCellStyling(
      std::vector<MantidQt::MantidWidgets::Batch::Cell> &cells);
  void clearInvalidCellStyling(MantidQt::MantidWidgets::Batch::Cell &cell);
  void applyInvalidCellStyling(MantidQt::MantidWidgets::Batch::Cell &cell);

  void
  removeGroupsFromView(std::vector<int> const &groupIndicesOrderedLowToHigh);
  void
  removeGroupsFromModel(std::vector<int> const &groupIndicesOrderedLowToHigh);
  void removeRowsFromModel(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> rows);
  void
  showAllCellsOnRowAsValid(MantidWidgets::Batch::RowLocation const &itemIndex);

  void removeRowsAndGroupsFromView(std::vector<
      MantidQt::MantidWidgets::Batch::RowLocation> const &locations);
  void removeRowsAndGroupsFromModel(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> locations);

  void appendRowsToGroupsInView(std::vector<int> const &groupIndices);
  void appendRowsToGroupsInModel(std::vector<int> const &groupIndices);
  void appendEmptyGroupInModel();
  void appendEmptyGroupInView();
  void insertEmptyGroupInModel(int beforeGroup);
  void insertEmptyGroupInView(int beforeGroup);
  void insertEmptyRowInModel(int groupIndex, int beforeRow);
  std::vector<std::string>
  cellTextFromViewAt(MantidWidgets::Batch::RowLocation const &location) const;
  void
  showCellsAsInvalidInView(MantidWidgets::Batch::RowLocation const &itemIndex,
                           std::vector<int> const &invalidColumns);
  void
  updateGroupName(MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex,
                  int column, std::string const &oldValue,
                  std::string const &newValue);
  void
  updateRowField(MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex,
                 int column, std::string const &oldValue,
                 std::string const &newValue);

  static auto constexpr DEPTH_LIMIT = 2;

  IRunsTableView *m_view;
  std::vector<std::string> m_instruments;
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
      m_clipboard;
  Jobs m_model;
  double m_thetaTolerance;
  JobsViewUpdater m_jobViewUpdater;
  WorkspaceNamesFactory m_workspaceNameFactory;
};

template <typename Row>
std::vector<MantidQt::MantidWidgets::Batch::Cell>
cellsFromGroup(Group<Row> const &group,
               MantidQt::MantidWidgets::Batch::Cell const &deadCell) {
  auto cells = std::vector<MantidQt::MantidWidgets::Batch::Cell>(9, deadCell);
  cells[0] = MantidQt::MantidWidgets::Batch::Cell(group.name());
  return cells;
}

template <typename WorkspaceNames>
std::vector<MantidQt::MantidWidgets::Batch::Cell>
cellsFromRow(Row<WorkspaceNames> const &row) {
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(std::to_string(row.theta())),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().first),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().second),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(
           map(row.qRange(),
               [](RangeInQ const &range) -> double { return range.min(); }))),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(
           map(row.qRange(),
               [](RangeInQ const &range) -> double { return range.max(); }))),
       MantidQt::MantidWidgets::Batch::Cell(optionalToString(
           map(row.qRange(),
               [](RangeInQ const &range) -> double { return range.step(); }))),
       MantidQt::MantidWidgets::Batch::Cell(
           optionalToString(row.scaleFactor())),
       MantidQt::MantidWidgets::Batch::Cell(
           MantidWidgets::optionsToString(row.reductionOptions()))});
}
}
}
#endif // MANTID_CUSTOMINTERFACES_RUNSTABLEPRESENTER_H_
