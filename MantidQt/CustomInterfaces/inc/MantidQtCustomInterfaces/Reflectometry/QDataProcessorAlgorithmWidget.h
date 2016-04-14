#ifndef MANTID_CUSTOMINTERFACES_QDATAPROCESSORALGORITHMWIDGET_H_
#define MANTID_CUSTOMINTERFACES_QDATAPROCESSORALGORITHMWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtCustomInterfaces/DllConfig.h"
#include "MantidQtCustomInterfaces/ProgressableView.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithmView.h"
#include "MantidQtCustomInterfaces/Reflectometry/GenericDataProcessorPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/QDataProcessorTableModel.h"
#include "ui_DataProcessorAlgorithmWidget.h"
#include <QSignalMapper>

namespace MantidQt {
namespace CustomInterfaces {

/** QDataProcessorAlgorithmWidget : Provides an interface for processing table
data.

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
class DLLExport QDataProcessorAlgorithmWidget
    : public MantidQt::API::MantidWidget,
      public DataProcessorAlgorithmView,
      public ProgressableView {

  Q_OBJECT
public:
  QDataProcessorAlgorithmWidget(QWidget *parent = 0);
  ~QDataProcessorAlgorithmWidget() override;

  // Connect the model
  void showTable(QDataProcessorTableModel_sptr model) override;

  // Dialog/Prompt methods
  std::string askUserString(const std::string &prompt, const std::string &title,
                            const std::string &defaultValue) override;
  bool askUserYesNo(std::string prompt, std::string title) override;
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
  void
  addHintingLineEdit(const std::string &title, const std::string &name,
                     const std::map<std::string, std::string> &hints) override;

  // Accessor methods
  std::set<int> getSelectedRows() const override;
  std::string getProcessInstrument() const override;
  std::string getPostprocessingInstructions() const override;
  std::string getWorkspaceToOpen() const override;
  std::string getClipboard() const override;

  boost::shared_ptr<DataProcessorPresenter> getTablePresenter() const override;

private:
  // initialise the interface
  void createTable();
  // Set a selected model (table workspace)
  void setModel(std::string name);

  // the presenter
  boost::shared_ptr<GenericDataProcessorPresenter> m_presenter;
  // the models
  QDataProcessorTableModel_sptr m_model;
  // the interface
  Ui::DataProcessorAlgorithmWidget ui;
  // the workspace the user selected to open
  std::string m_toOpen;
  QSignalMapper *m_openMap;

signals:
  void comboProcessInstrument_currentIndexChanged(int index);

public slots:
  void on_comboProcessInstrument_currentIndexChanged(int index);

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
  void on_actionImportTable_triggered();
  void on_actionExportTable_triggered();
  void on_actionHelp_triggered();
  void on_actionPlotRow_triggered();
  void on_actionPlotGroup_triggered();

  void setModel(QString name);
  void tableUpdated(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void showContextMenu(const QPoint &pos);
};

} // namespace Mantid
} // namespace CustomInterfaces

#endif /* MANTID_CUSTOMINTERFACES_QDATAPROCESSORALGORITHMWIDGET_H_ */