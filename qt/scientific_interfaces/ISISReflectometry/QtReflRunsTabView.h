#ifndef MANTID_ISISREFLECTOMETRY_QTREFLRUNSTABVIEW_H_
#define MANTID_ISISREFLECTOMETRY_QTREFLRUNSTABVIEW_H_

#include "DllConfig.h"
#include "IReflRunsTabView.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/ProgressableView.h"

#include "ui_ReflRunsTabWidget.h"

#include <QBasicTimer>

namespace MantidQt {

namespace MantidWidgets {
namespace DataProcessor {
// Forward decs
class Command;
class QtCommandAdapter;
} // namespace DataProcessor
class SlitCalculator;
} // namespace MantidWidgets
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

// Forward decs
class IReflRunsTabPresenter;
class ReflSearchModel;

using MantidWidgets::SlitCalculator;
namespace DataProcessor = MantidWidgets::DataProcessor;

/** QtReflRunsTabView : Provides an interface for the "Runs" tab in the
ISIS Reflectometry interface.

Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL QtReflRunsTabView
    : public MantidQt::API::MantidWidget,
      public IReflRunsTabView,
      public MantidQt::MantidWidgets::ProgressableView {
  Q_OBJECT
public:
  /// Constructor
  QtReflRunsTabView(QWidget *parent = nullptr);
  /// Destructor
  ~QtReflRunsTabView() override;
  // Connect the model
  void showSearch(boost::shared_ptr<ReflSearchModel> model) override;

  // Setter methods
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &defaultInstrument) override;
  void setTransferMethods(const std::set<std::string> &methods) override;
  void setTableCommands(std::vector<std::unique_ptr<DataProcessor::Command>>
                            tableCommands) override;
  void setRowCommands(std::vector<std::unique_ptr<DataProcessor::Command>>
                          rowCommands) override;
  void clearCommands() override;
  void updateMenuEnabledState(bool isProcessing) override;
  void setAutoreduceButtonEnabled(bool enabled) override;
  void setAutoreducePauseButtonEnabled(bool enabled) override;
  void setTransferButtonEnabled(bool enabled) override;
  void setInstrumentComboEnabled(bool enabled) override;
  void setTransferMethodComboEnabled(bool enabled) override;
  void setSearchTextEntryEnabled(bool enabled) override;
  void setSearchButtonEnabled(bool enabled) override;
  void setStartMonitorButtonEnabled(bool enabled) override;
  void setStopMonitorButtonEnabled(bool enabled) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Accessor methods
  std::set<int> getSelectedSearchRows() const override;
  std::set<int> getAllSearchRows() const override;
  std::string getSearchInstrument() const override;
  std::string getSearchString() const override;
  std::string getTransferMethod() const override;
  int getSelectedGroup() const override;

  IReflRunsTabPresenter *getPresenter() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getMonitorAlgorithmRunner() const override;

  // Timer methods
  void startTimer(const int millisecs) override;
  void stopTimer() override;

  // Start an ICAT search
  void startIcatSearch() override;

  // Live data monitor
  void startMonitor() override;
  void stopMonitor() override;

private:
  /// initialise the interface
  void initLayout();
  // Adds an action (command) to a menu
  void addToMenu(QMenu *menu, std::unique_ptr<DataProcessor::Command> command);
  // Implement our own timer event to trigger autoreduction
  void timerEvent(QTimerEvent *event) override;

  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_algoRunner;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_monitorAlgoRunner;

  // the presenter
  std::shared_ptr<IReflRunsTabPresenter> m_presenter;
  // the search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  // Command adapters
  std::vector<std::unique_ptr<DataProcessor::QtCommandAdapter>> m_commands;
  // the interface (uses actions owned by m_commands)
  Ui::ReflRunsTabWidget ui;
  // the slit calculator
  SlitCalculator *m_calculator;
  // Timer for triggering periodic autoreduction
  QBasicTimer m_timer;

private slots:
  void on_actionSearch_triggered();
  void on_actionAutoreduce_triggered();
  void on_actionAutoreducePause_triggered();
  void on_actionTransfer_triggered();
  void slitCalculatorTriggered();
  void icatSearchComplete();
  void startMonitorComplete();
  void instrumentChanged(int index);
  void groupChanged();
  void showSearchContextMenu(const QPoint &pos);
  void on_buttonMonitor_clicked();
  void on_buttonStopMonitor_clicked();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTID_ISISREFLECTOMETRY_QTREFLRUNSTABVIEW_H_ */
