// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/RunsTable/QtRunsTableView.h"
#include "IRunsView.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "QtSearchModel.h"

#include "ui_RunsWidget.h"
#include <QBasicTimer>

namespace MantidQt {

namespace API {
class QtAlgorithmRunner;
}

namespace CustomInterfaces {
namespace ISISReflectometry {

/** QtRunsView : Provides an interface for the "Runs" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtRunsView : public MantidQt::API::MantidWidget, public IRunsView {
  Q_OBJECT
public:
  QtRunsView(QWidget *parent, const RunsTableViewFactory &makeView);

  void subscribe(RunsViewSubscriber *notifyee) override;
  void subscribeTimer(RunsViewTimerSubscriber *notifyee) override;
  void subscribeSearch(RunsViewSearchSubscriber *notifyee) override;
  IRunsTableView *table() const override;

  // Timer methods
  void startTimer(const int millisecs) override;
  void stopTimer() override;

  // Search methods
  void resizeSearchResultsColumnsToContents() override;
  int getSearchResultsTableWidth() const override;
  int getSearchResultsColumnWidth(int column) const override;
  void setSearchResultsColumnWidth(int column, int width) override;
  ISearchModel const &searchResults() const override;
  ISearchModel &mutableSearchResults() override;

  // Setter methods
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &selectedInstrument = "") override;
  void updateMenuEnabledState(bool isProcessing) override;
  void setAutoreduceButtonEnabled(bool enabled) override;
  void setAutoreducePauseButtonEnabled(bool enabled) override;
  void setTransferButtonEnabled(bool enabled) override;
  void setInstrumentComboEnabled(bool enabled) override;
  void setSearchTextEntryEnabled(bool enabled) override;
  void setSearchButtonEnabled(bool enabled) override;
  void setSearchResultsEnabled(bool enabled) override;
  void setStartMonitorButtonEnabled(bool enabled) override;
  void setStopMonitorButtonEnabled(bool enabled) override;
  void setUpdateIntervalSpinBoxEnabled(bool enabled) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Accessor methods
  std::set<int> getSelectedSearchRows() const override;
  std::set<int> getAllSearchRows() const override;
  std::string getSearchInstrument() const override;
  void setSearchInstrument(std::string const &instrumentName) override;
  std::string getSearchString() const override;
  std::string getSearchCycle() const override;
  int getLiveDataUpdateInterval() const override;

  std::shared_ptr<MantidQt::API::QtAlgorithmRunner> getAlgorithmRunner() const override;
  std::shared_ptr<MantidQt::API::QtAlgorithmRunner> getMonitorAlgorithmRunner() const override;

  // Live data monitor
  void startMonitor() override;
  void stopMonitor() override;

private:
  /// initialise the interface
  void initLayout();
  /// Implement our own timer event to trigger autoreduction
  void timerEvent(QTimerEvent *event) override;

  std::shared_ptr<MantidQt::API::QtAlgorithmRunner> m_algoRunner;
  std::shared_ptr<MantidQt::API::QtAlgorithmRunner> m_monitorAlgoRunner;

  void setSelected(QComboBox &box, std::string const &str);

  RunsViewSubscriber *m_notifyee;
  RunsViewTimerSubscriber *m_timerNotifyee;
  RunsViewSearchSubscriber *m_searchNotifyee;

  QtSearchModel m_searchModel;

  // the interface
  Ui::RunsWidget m_ui;

  QtRunsTableView *m_tableView;

  // Timer for triggering periodic autoreduction
  QBasicTimer m_timer;

  friend class Encoder;
  friend class Decoder;
  friend class CoderCommonTester;

private slots:
  void on_actionSearch_triggered();
  void on_actionAutoreduce_triggered();
  void on_actionAutoreducePause_triggered();
  void on_actionTransfer_triggered();
  void on_actionExport_triggered();
  void on_buttonMonitor_clicked();
  void on_buttonStopMonitor_clicked();
  void onStartMonitorComplete();
  void onSearchResultsChanged(const QModelIndex &, const QModelIndex &);
  void onSearchComplete();
  void onInstrumentChanged(int index);
  void onShowSearchContextMenuRequested(const QPoint &pos);
};

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
