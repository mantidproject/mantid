#ifndef MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#include "DllConfig.h"
#include <memory>
#include "IBatchView.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "Reduction/Group.h"
#include "Reduction/ReductionJobs.h"
#include <sstream>

namespace MantidQt {
namespace CustomInterfaces {

template <typename Group>
std::vector<MantidQt::MantidWidgets::Batch::Cell>
cellsFromGroup(Group const &group,
               MantidQt::MantidWidgets::Batch::Cell const &deadCell) {
  auto cells = std::vector<MantidQt::MantidWidgets::Batch::Cell>(9, deadCell);
  cells[0] = MantidQt::MantidWidgets::Batch::Cell(group.name());
  return cells;
}

inline std::string
reductionOptionsToString(std::map<std::string, std::string> const &options) {
  if (!options.empty()) {
    auto resultStream = std::ostringstream();
    auto optionsKvpIt = options.cbegin();

    auto const &firstKvp = (*optionsKvpIt);
    resultStream << firstKvp.first << "='" << firstKvp.second << '\'';

    for (; optionsKvpIt != options.cend(); ++optionsKvpIt) {
      auto kvp = (*optionsKvpIt);
      resultStream << ", " << kvp.first << "='" << kvp.second << '\'';
    }

    return resultStream.str();
  } else {
    return std::string();
  }
}

template <typename Row>
std::vector<MantidQt::MantidWidgets::Batch::Cell> cellsFromRow(Row const &row) {
  return std::vector<MantidQt::MantidWidgets::Batch::Cell>(
      {MantidQt::MantidWidgets::Batch::Cell(boost::join(row.runNumbers(), "+")),
       MantidQt::MantidWidgets::Batch::Cell(std::to_string(row.theta())),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().first),
       MantidQt::MantidWidgets::Batch::Cell(
           row.transmissionWorkspaceNames().second),
       MantidQt::MantidWidgets::Batch::Cell(
           row.qRange().is_initialized()
               ? std::to_string(row.qRange().get().min())
               : std::string()),
       MantidQt::MantidWidgets::Batch::Cell(
           row.qRange().is_initialized()
               ? std::to_string(row.qRange().get().max())
               : std::string()),
       MantidQt::MantidWidgets::Batch::Cell(
           row.qRange().is_initialized()
               ? std::to_string(row.qRange().get().step())
               : std::string()),
       MantidQt::MantidWidgets::Batch::Cell(
           row.scaleFactor().is_initialized()
               ? std::to_string(row.scaleFactor().get())
               : std::string()),
       MantidQt::MantidWidgets::Batch::Cell(
           reductionOptionsToString(row.reductionOptions()))});
}

class BatchViewJobsUpdater {
public:
  BatchViewJobsUpdater(MantidQt::MantidWidgets::Batch::IJobTreeView &view)
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
  bool isGroupLocation(MantidWidgets::Batch::RowLocation const &location) const;
  bool isRowLocation(MantidWidgets::Batch::RowLocation const &location) const;
  int groupOf(MantidWidgets::Batch::RowLocation const &location) const;
  int rowOf(MantidWidgets::Batch::RowLocation const &location) const;
  void applyGroupStyling(MantidWidgets::Batch::RowLocation const &location);

  bool containsGroups(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &locations)
      const;
  std::vector<int> groupIndexesFromSelection(
      std::vector<MantidWidgets::Batch::RowLocation> const &selected) const;

  std::vector<int> mapToContainingGroups(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const &
          mustNotContainRoot) const;
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
  mapToContentText(std::vector<MantidWidgets::Batch::Cell> const &cells) const;
  std::vector<std::string>
  cellTextFromViewAt(MantidWidgets::Batch::RowLocation const &location) const;
  void
  showCellsAsInvalidInView(MantidWidgets::Batch::RowLocation const &itemIndex,
                           std::vector<int> const &invalidColumns);

  static auto constexpr DEPTH_LIMIT = 2;

  IBatchView *m_view;
  std::vector<std::string> m_instruments;
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
      m_clipboard;
  Jobs m_model;
  BatchViewJobsUpdater m_jobViewUpdater;
  WorkspaceNamesFactory m_workspaceNameFactory;
};

}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
