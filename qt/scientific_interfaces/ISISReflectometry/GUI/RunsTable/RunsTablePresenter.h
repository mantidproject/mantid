// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CUSTOMINTERFACES_RUNSTABLEPRESENTER_H_
#define MANTID_CUSTOMINTERFACES_RUNSTABLEPRESENTER_H_
#include "../Runs/IRunsPresenter.h"
#include "DllConfig.h"
#include "IRunsTablePresenter.h"
#include "IRunsTableView.h"
#include "JobsViewUpdater.h"
#include "MantidQtWidgets/Common/Batch/IJobTreeView.h"
#include "Reduction/Group.h"
#include "Reduction/ReductionJobs.h"
#include <memory>

namespace MantidQt {
namespace CustomInterfaces {

class MANTIDQT_ISISREFLECTOMETRY_DLL RunsTablePresenter
    : public IRunsTablePresenter,
      public RunsTableViewSubscriber {
public:
  RunsTablePresenter(IRunsTableView *view,
                     std::vector<std::string> const &instruments,
                     double thetaTolerance, ReductionJobs reductionJobs);

  void notifyRemoveAllRowsAndGroupsRequested();

  // IRunsTablePresenter overrides
  void acceptMainPresenter(IRunsPresenter *mainPresenter) override;
  ReductionJobs const &reductionJobs() const override;
  void mergeAdditionalJobs(ReductionJobs const &jobs) override;
  void instrumentChanged(std::string const &instrumentName) override;
  void settingsChanged() override;

  // RunsTableViewSubscriber overrides
  void notifyReductionResumed() override;
  void notifyReductionPaused() override;
  void notifyInsertRowRequested() override;
  void notifyInsertGroupRequested() override;
  void notifyDeleteRowRequested() override;
  void notifyDeleteGroupRequested() override;
  void notifyFilterChanged(std::string const &filterValue) override;
  void notifyInstrumentChanged() override;
  void notifyExpandAllRequested() override;
  void notifyCollapseAllRequested() override;
  void reductionPaused() override;
  void reductionResumed() override;
  void autoreductionPaused() override;
  void autoreductionResumed() override;

  // JobTreeViewSubscriber overrides
  void notifyCellTextChanged(
      MantidQt::MantidWidgets::Batch::RowLocation const &itemIndex, int column,
      std::string const &oldValue, std::string const &newValue) override;
  void notifyRowInserted(MantidQt::MantidWidgets::Batch::RowLocation const
                             &newRowLocation) override;
  void notifyRemoveRowsRequested(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const
          &locationsOfRowsToRemove) override;
  void notifyCutRowsRequested() override;
  void notifyCopyRowsRequested() override;
  void notifyPasteRowsRequested() override;
  void notifyFilterReset() override;

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

  void removeRowsAndGroupsFromView(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> const
          &locations);
  void removeAllRowsAndGroupsFromView();
  void removeRowsAndGroupsFromModel(
      std::vector<MantidQt::MantidWidgets::Batch::RowLocation> locations);
  void removeAllRowsAndGroupsFromModel();

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
  void updateWidgetEnabledState(bool isProcessing);

  static auto constexpr DEPTH_LIMIT = 2;

  IRunsTableView *m_view;
  std::vector<std::string> m_instruments;
  boost::optional<std::vector<MantidQt::MantidWidgets::Batch::Subtree>>
      m_clipboard;
  ReductionJobs m_model;
  double m_thetaTolerance;
  JobsViewUpdater m_jobViewUpdater;
  IRunsPresenter *m_mainPresenter;
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_RUNSTABLEPRESENTER_H_
