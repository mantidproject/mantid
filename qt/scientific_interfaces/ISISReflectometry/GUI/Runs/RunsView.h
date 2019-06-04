// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_RUNSTABVIEW_H_
#define MANTID_ISISREFLECTOMETRY_RUNSTABVIEW_H_

#include "Common/DllConfig.h"
#include "GUI/RunsTable/RunsTableView.h"
#include "IRunsView.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include "ui_RunsWidget.h"

#include <QBasicTimer>

namespace MantidQt {

namespace MantidWidgets {
namespace DataProcessor {
// Forward decs
class Command;
} // namespace DataProcessor
class SlitCalculator;
} // namespace MantidWidgets
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

// Forward decs
class SearchModel;

using MantidWidgets::SlitCalculator;
namespace DataProcessor = MantidWidgets::DataProcessor;

/** RunsView : Provides an interface for the "Runs" tab in the
ISIS Reflectometry interface.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL RunsView
    : public MantidQt::API::MantidWidget,
      public IRunsView {
  Q_OBJECT
public:
  RunsView(QWidget *parent, RunsTableViewFactory makeView);

  void subscribe(RunsViewSubscriber *notifyee) override;
  IRunsTableView *table() const override;

  // Connect the model
  void showSearch(boost::shared_ptr<SearchModel> model) override;

  // Setter methods
  void setInstrumentList(const std::vector<std::string> &instruments,
                         int defaultInstrumentIndex) override;
  void updateMenuEnabledState(bool isProcessing) override;
  void setAutoreduceButtonEnabled(bool enabled) override;
  void setAutoreducePauseButtonEnabled(bool enabled) override;
  void setTransferButtonEnabled(bool enabled) override;
  void setInstrumentComboEnabled(bool enabled) override;
  void setSearchTextEntryEnabled(bool enabled) override;
  void setSearchButtonEnabled(bool enabled) override;
  void setStartMonitorButtonEnabled(bool enabled) override;
  void setStopMonitorButtonEnabled(bool enabled) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;
  void loginFailed(std::string const &fullError) override;

  // Accessor methods
  std::set<int> getSelectedSearchRows() const override;
  std::set<int> getAllSearchRows() const override;
  std::string getSearchInstrument() const override;
  void setSearchInstrument(std::string const &instrumentName) override;
  std::string getSearchString() const override;

  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getMonitorAlgorithmRunner() const override;

  // Timer methods
  void startTimer(const int millisecs) override;
  void stopTimer() override;

  // Start an ICAT search
  void startIcatSearch() override;
  void noActiveICatSessions() override;
  void missingRunsToTransfer() override;

  // Live data monitor
  void startMonitor() override;
  void stopMonitor() override;

private:
  /// initialise the interface
  void initLayout();
  // Implement our own timer event to trigger autoreduction
  void timerEvent(QTimerEvent *event) override;

  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_algoRunner;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_monitorAlgoRunner;

  void setSelected(QComboBox &box, std::string const &str);

  RunsViewSubscriber *m_notifyee;

  boost::shared_ptr<SearchModel> m_searchModel;

  // the interface
  Ui::RunsWidget ui;
  // the slit calculator
  SlitCalculator *m_calculator;
  // Timer for triggering periodic autoreduction
  QBasicTimer m_timer;

  RunsTableView *m_tableView;

private slots:
  void on_actionSearch_triggered();
  void on_actionAutoreduce_triggered();
  void on_actionAutoreducePause_triggered();
  void on_actionTransfer_triggered();
  void on_buttonMonitor_clicked();
  void on_buttonStopMonitor_clicked();
  void onStartMonitorComplete();
  void onSearchComplete();
  void onInstrumentChanged(int index);
  void onShowSlitCalculatorRequested();
  void onShowSearchContextMenuRequested(const QPoint &pos);
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_RUNSVIEW_H_ */
