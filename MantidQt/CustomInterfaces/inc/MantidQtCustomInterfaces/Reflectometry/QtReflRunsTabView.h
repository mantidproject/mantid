#ifndef MANTID_CUSTOMINTERFACES_QTREFLRUNSTABVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLRUNSTABVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabView.h"
#include "MantidQtMantidWidgets/ProgressableView.h"

#include "ui_ReflRunsTabWidget.h"

namespace MantidQt {

namespace MantidWidgets {
// Forward decs
class DataProcessorCommand;
class DataProcessorCommandAdapter;
class SlitCalculator;
}
namespace API {
class AlgorithmRunner;
}

namespace CustomInterfaces {

// Forward decs
class IReflRunsTabPresenter;
class ReflSearchModel;

using MantidWidgets::DataProcessorCommand;
using MantidWidgets::DataProcessorCommandAdapter;
using MantidWidgets::SlitCalculator;

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
class MANTIDQT_CUSTOMINTERFACES_DLL QtReflRunsTabView
    : public MantidQt::API::MantidWidget,
      public IReflRunsTabView,
      public MantidQt::MantidWidgets::ProgressableView {
  Q_OBJECT
public:
  /// Constructor
  QtReflRunsTabView(QWidget *parent = 0);
  /// Destructor
  ~QtReflRunsTabView() override;
  // Connect the model
  void showSearch(boost::shared_ptr<ReflSearchModel> model) override;

  // Setter methods
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &defaultInstrument) override;
  void setTransferMethods(const std::set<std::string> &methods) override;
  void setTableCommands(std::vector<std::unique_ptr<DataProcessorCommand>>
                            tableCommands) override;
  void setRowCommands(
      std::vector<std::unique_ptr<DataProcessorCommand>> rowCommands) override;
  void clearCommands() override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Accessor methods
  std::set<int> getSelectedSearchRows() const override;
  std::string getSearchInstrument() const override;
  std::string getSearchString() const override;
  std::string getTransferMethod() const override;
  int getSelectedGroup() const override;

  IReflRunsTabPresenter *getPresenter() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const override;

private:
  /// initialise the interface
  void initLayout();
  // Adds an action (command) to a menu
  void addToMenu(QMenu *menu, std::unique_ptr<DataProcessorCommand> command);

  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_algoRunner;

  // the presenter
  std::shared_ptr<IReflRunsTabPresenter> m_presenter;
  // the search model
  boost::shared_ptr<ReflSearchModel> m_searchModel;
  // the interface
  Ui::ReflRunsTabWidget ui;
  // the slit calculator
  SlitCalculator *m_calculator;
  // Command adapters
  std::vector<std::unique_ptr<DataProcessorCommandAdapter>> m_commands;

private slots:
  void on_actionSearch_triggered();
  void on_actionTransfer_triggered();
  void slitCalculatorTriggered();
  void icatSearchComplete();
  void instrumentChanged(int index);
  void groupChanged();
  void showSearchContextMenu(const QPoint &pos);
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLRUNSTABVIEW_H_ */
