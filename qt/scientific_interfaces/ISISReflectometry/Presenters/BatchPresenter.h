#ifndef MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#include "DllConfig.h"
#include <memory>
#include "Views/IBatchView.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "Reduction/Group.h"
#include "Reduction/ReductionJobs.h"
#include "JobViewUpdater.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "Map.h"

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchPresenter
    : public BatchViewSubscriber {
public:
  BatchPresenter(IBatchView *view, std::vector<std::string> const &instruments,
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
  void applyGroupStyling(MantidWidgets::Batch::RowLocation const &location);
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

  IBatchView *m_view;
  std::vector<std::string> m_instruments;
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
      m_clipboard;
  Jobs m_model;
  BatchViewJobsUpdater m_jobViewUpdater;
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
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
