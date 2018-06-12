#ifndef MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
#include "DllConfig.h"
#include <memory>
#include "IBatchView.h"
#include "Reduction/Group.h"
#include "Reduction/ReductionJobs.h"

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

  void notifyInsertRowRequested() override;
  void notifyInsertGroupRequested() override;
  void notifyDeleteRowRequested() override;
  void notifyDeleteGroupRequested() override;

  Jobs const& reductionJobs() const;
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
};

class MANTIDQT_ISISREFLECTOMETRY_DLL BatchPresenterFactory {
public:
  BatchPresenterFactory(std::vector<std::string> const &instruments);
  std::unique_ptr<BatchPresenter> operator()(IBatchView *view) const;

private:
  std::vector<std::string> m_instruments;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_BATCHVIEWPRESENTER_H_
