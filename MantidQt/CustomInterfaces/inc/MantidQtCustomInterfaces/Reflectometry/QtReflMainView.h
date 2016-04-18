#ifndef MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/ProgressableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtMantidWidgets/SlitCalculator.h"
#include "ui_ReflMainWidget.h"

namespace MantidQt {
namespace CustomInterfaces {
// Forward dec
class ReflCommandAdapter;
using ReflCommandAdapter_uptr = std::unique_ptr<ReflCommandAdapter>;

/** QtReflMainView : Provides an interface for processing reflectometry data.

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
class DLLExport QtReflMainView : public MantidQt::API::UserSubWindow,
                                 public ReflMainView,
                                 public ProgressableView {
  Q_OBJECT
public:
  QtReflMainView(QWidget *parent = 0);
  ~QtReflMainView() override;
  /// Name of the interface
  static std::string name() { return "ISIS Reflectometry (Polref)"; }
  // This interface's categories.
  static QString categoryInfo() { return "Reflectometry"; }
  // Connect the model
  void showSearch(ReflSearchModel_sptr model) override;

  // Dialog/Prompt methods
  std::string askUserString(const std::string &prompt, const std::string &title,
                            const std::string &defaultValue) override;
  void giveUserInfo(std::string prompt, std::string title) override;
  void giveUserCritical(std::string prompt, std::string title) override;
  void showAlgorithmDialog(const std::string &algorithm) override;

  // Setter methods
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &defaultInstrument) override;
  void setTransferMethods(const std::set<std::string> &methods) override;
  void setTableCommands(std::vector<ReflCommand_uptr> tableCommands) override;
  void setRowCommands(std::vector<ReflCommand_uptr> rowCommands) override;
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

  boost::shared_ptr<IReflPresenter> getPresenter() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const override;

private:
  // initialise the interface
  void initLayout() override;
  // Adds an action (command) to a menu
  void addToMenu(QMenu *menu, ReflCommand_uptr command);

  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_algoRunner;

  // the presenter
  boost::shared_ptr<IReflPresenter> m_presenter;
  // the search model
  ReflSearchModel_sptr m_searchModel;
  // the interface
  Ui::reflMainWidget ui;
  // the slit calculator
  MantidWidgets::SlitCalculator *m_calculator;
  // Command adapters
  std::vector<ReflCommandAdapter_uptr> m_commands;

private slots:
  void on_actionSearch_triggered();
  void on_actionTransfer_triggered();
  void on_actionHelp_triggered();
  void slitCalculatorTriggered();
  void icatSearchComplete();

  void showSearchContextMenu(const QPoint &pos);
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_ */