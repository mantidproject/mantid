#ifndef MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_
#define MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/ProgressableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSearchModel.h"
#include "MantidQtMantidWidgets/SlitCalculator.h"
#include "ui_ReflMainWidget.h"
#include <QSignalMapper>
#include <boost/scoped_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {

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
                                 public ReflTableView,
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
  void showTable(QReflTableModel_sptr model) override;
  void showSearch(ReflSearchModel_sptr model) override;

  // Dialog/Prompt methods
  std::string askUserString(const std::string &prompt, const std::string &title,
                            const std::string &defaultValue) override;
  bool askUserYesNo(std::string prompt, std::string title) override;
  void giveUserInfo(std::string prompt, std::string title) override;
  void giveUserWarning(std::string prompt, std::string title) override;
  void giveUserCritical(std::string prompt, std::string title) override;
  void showAlgorithmDialog(const std::string &algorithm) override;
  void showImportDialog() override;
  std::string requestNotebookPath() override;

  // Settings
  void saveSettings(const std::map<std::string, QVariant> &options) override;
  void loadSettings(std::map<std::string, QVariant> &options) override;

  // Plotting
  void plotWorkspaces(const std::set<std::string> &workspaces) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Get status of the checkbox which dictates whether an ipython notebook is
  // produced
  bool getEnableNotebook() override;

  // Settor methods
  void setSelection(const std::set<int> &rows) override;
  void setTableList(const std::set<std::string> &tables) override;
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &defaultInstrument) override;
  void setOptionsHintStrategy(
      MantidQt::MantidWidgets::HintStrategy *hintStrategy) override;
  void setClipboard(const std::string &text) override;
  void setTransferMethods(const std::set<std::string> &methods) override;

  // Accessor methods
  std::set<int> getSelectedRows() const override;
  std::set<int> getSelectedSearchRows() const override;
  std::string getSearchInstrument() const override;
  std::string getProcessInstrument() const override;
  std::string getWorkspaceToOpen() const override;
  std::string getClipboard() const override;
  std::string getSearchString() const override;
  std::string getTransferMethod() const override;

  boost::shared_ptr<IReflPresenter> getPresenter() const override;
  boost::shared_ptr<MantidQt::API::AlgorithmRunner>
  getAlgorithmRunner() const override;

private:
  // initialise the interface
  void initLayout() override;

  boost::shared_ptr<MantidQt::API::AlgorithmRunner> m_algoRunner;

  // the presenter
  boost::shared_ptr<IReflPresenter> m_presenter;
  // the models
  QReflTableModel_sptr m_model;
  ReflSearchModel_sptr m_searchModel;
  // the interface
  Ui::reflMainWidget ui;
  // the workspace the user selected to open
  std::string m_toOpen;
  QSignalMapper *m_openMap;
  MantidWidgets::SlitCalculator *m_calculator;

private slots:
  void on_actionNewTable_triggered();
  void on_actionSaveTable_triggered();
  void on_actionSaveTableAs_triggered();
  void on_actionAppendRow_triggered();
  void on_actionPrependRow_triggered();
  void on_actionDeleteRow_triggered();
  void on_actionProcess_triggered();
  void on_actionGroupRows_triggered();
  void on_actionClearSelected_triggered();
  void on_actionCopySelected_triggered();
  void on_actionCutSelected_triggered();
  void on_actionPasteSelected_triggered();
  void on_actionExpandSelection_triggered();
  void on_actionOptionsDialog_triggered();
  void on_actionSearch_triggered();
  void on_actionTransfer_triggered();
  void on_actionImportTable_triggered();
  void on_actionExportTable_triggered();
  void on_actionHelp_triggered();
  void on_actionPlotRow_triggered();
  void on_actionPlotGroup_triggered();
  void on_actionSlitCalculator_triggered();
  void icatSearchComplete();

  void on_comboSearchInstrument_currentIndexChanged(int index);
  void on_comboProcessInstrument_currentIndexChanged(int index);

  void setModel(QString name);
  void tableUpdated(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void showContextMenu(const QPoint &pos);
  void showSearchContextMenu(const QPoint &pos);
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QTREFLMAINVIEW_H_ */