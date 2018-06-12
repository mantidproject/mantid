#ifndef MANTID_ISISREFLECTOMETRY_QTREFLRUNSTABVIEW_H_
#define MANTID_ISISREFLECTOMETRY_QTREFLRUNSTABVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "DllConfig.h"
#include "IReflRunsTabView.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include "BatchPresenter.h"
#include "BatchView.h"

#include "ui_ReflRunsTabWidget.h"

namespace MantidQt {

namespace MantidWidgets {
namespace DataProcessor {
// Forward decs
class Command;
class QtCommandAdapter;
}
class SlitCalculator;
}
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
  QtReflRunsTabView(QWidget *parent, BatchViewFactory makeView);
  ~QtReflRunsTabView() override;

  void subscribe(IReflRunsTabPresenter* presenter) override;
  std::vector<IBatchView*> const& tableViews() const override;

  // Connect the model
  void showSearch(boost::shared_ptr<ReflSearchModel> model) override;

  // Setter methods
  void setInstrumentList(const std::vector<std::string> &instruments,
                         int defaultInstrumentIndex) override;
  void setTableCommands(std::vector<std::unique_ptr<DataProcessor::Command>>
                            tableCommands) override;
  void setRowCommands(std::vector<std::unique_ptr<DataProcessor::Command>>
                          rowCommands) override;
  void setAllSearchRowsSelected() override;
  void clearCommands() override;
  void updateMenuEnabledState(bool isProcessing) override;
  void setAutoreduceButtonEnabled(bool enabled) override;
  void setTransferButtonEnabled(bool enabled) override;
  void setInstrumentComboEnabled(bool enabled) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Accessor methods
  std::set<int> getSelectedSearchRows() const override;
  std::string getSearchInstrument() const override;
  std::string getSearchString() const override;
  int getSelectedGroup() const override;

  IReflRunsTabPresenter *getPresenter() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const override;

private:
  /// initialise the interface
  void initLayout();
  // Adds an action (command) to a menu
  void addToMenu(QMenu *menu, std::unique_ptr<DataProcessor::Command> command);

  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_algoRunner;
  // the presenter
  IReflRunsTabPresenter* m_presenter;

  // the search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  // Command adapters
  std::vector<std::unique_ptr<DataProcessor::QtCommandAdapter>> m_commands;
  // the interface (uses actions owned by m_commands)
  Ui::ReflRunsTabWidget ui;
  // the slit calculator
  SlitCalculator *m_calculator;

  std::vector<IBatchView*> m_tableViews;

  BatchViewFactory m_makeBatchView;

private slots:
  void on_actionSearch_triggered();
  void on_actionAutoreduce_triggered();
  void on_actionTransfer_triggered();
  void slitCalculatorTriggered();
  void icatSearchComplete();
  void instrumentChanged(int index);
  void groupChanged();
  void showSearchContextMenu(const QPoint &pos);
  void newAutoreduction();
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_ISISREFLECTOMETRY_QTREFLRUNSTABVIEW_H_ */
