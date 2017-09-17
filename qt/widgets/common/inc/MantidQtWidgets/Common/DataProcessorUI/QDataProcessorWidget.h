#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AbstractDataProcessorTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAction.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include "ui_DataProcessorWidget.h"
#include <QSignalMapper>

namespace MantidQt {
namespace MantidWidgets {

class DataProcessorCommandAdapter;
class DataProcessorMainPresenter;
class DataProcessorPreprocessMap;
class DataProcessorProcessingAlgorithm;
class DataProcessorPostprocessingAlgorithm;
class DataProcessorWhiteList;

/** QDataProcessorWidget : Provides an interface for processing table
data.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class EXPORT_OPT_MANTIDQT_COMMON QDataProcessorWidget
    : public MantidQt::API::MantidWidget,
      public DataProcessorView,
      public ProgressableView {

  Q_OBJECT
public:
  QDataProcessorWidget(std::unique_ptr<DataProcessorPresenter> presenter,
                       QWidget *parent = 0);
  QDataProcessorWidget(const DataProcessorWhiteList &, QWidget *parent);
  QDataProcessorWidget(const DataProcessorWhiteList &,
                       const DataProcessorProcessingAlgorithm &,
                       QWidget *parent);
  QDataProcessorWidget(const DataProcessorWhiteList &,
                       const DataProcessorPreprocessMap &,
                       const DataProcessorProcessingAlgorithm &,
                       QWidget *parent);
  QDataProcessorWidget(const DataProcessorWhiteList &,
                       const DataProcessorProcessingAlgorithm &,
                       const DataProcessorPostprocessingAlgorithm &,
                       QWidget *parent);
  QDataProcessorWidget(const DataProcessorWhiteList &,
                       const DataProcessorPreprocessMap &,
                       const DataProcessorProcessingAlgorithm &,
                       const DataProcessorPostprocessingAlgorithm &,
                       QWidget *parent);
  ~QDataProcessorWidget() override;

  // Add actions to the toolbar and context menu
  void addEditActions(
      const std::vector<std::unique_ptr<DataProcessorCommand>>& commands) override;

  // Connect the model
  void
  showTable(boost::shared_ptr<AbstractDataProcessorTreeModel> model) override;

  // Dialog/Prompt methods
  QString requestNotebookPath() override;
  /// Dialog/Prompt methods
  QString askUserString(const QString &prompt, const QString &title,
                        const QString &defaultValue) override;
  bool askUserYesNo(QString prompt, QString title) override;
  void giveUserWarning(QString prompt, QString title) override;
  void giveUserCritical(QString prompt, QString title) override;
  QString runPythonAlgorithm(const QString &pythonCode) override;

  // Settings
  void saveSettings(const std::map<QString, QVariant> &options) override;
  void loadSettings(std::map<QString, QVariant> &options) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Get status of the checkbox which dictates whether an ipython notebook is
  // produced
  bool isNotebookEnabled() override;

  // Expand/Collapse all groups
  void expandAll() override;
  void collapseAll() override;

  // Select all rows/groups
  void selectAll() override;

  // Setter methods
  void setSelection(const std::set<int> &groups) override;
  void setTableList(const QSet<QString> &tables) override;
  void setInstrumentList(const QString &instruments,
                         const QString &defaultInstrument) override;
  void
  setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy *hintStrategy,
                         int column) override;
  void setClipboard(const QString &text) override;

  // Transfer runs
  void transfer(const QList<QString> &runs);

  // Accessor methods
  std::map<int, std::set<int>> getSelectedChildren() const override;
  std::set<int> getSelectedParents() const override;
  QString getProcessInstrument() const override;
  QString getWorkspaceToOpen() const override;
  QString getClipboard() const override;

  DataProcessorPresenter *getPresenter() const override;
  QString getCurrentInstrument() const override;

  // Forward a main presenter to this view's presenter
  void accept(DataProcessorMainPresenter *);

  // Force re-processing of rows
  void setForcedReProcessing(bool forceReProcessing) override;

  void disableProcessButton() override;
  void enableProcessButton() override;

  void enableAction(int indexToEnable) override;
  void disableAction(int indexToDisable) override;

  void disableSelectionAndEditing() override;
  void enableSelectionAndEditing() override;
  // Get value in a cell
  QString getCell(int row, int column, int parentRow = 0, int parentColumn = 0);
  // Set value in a cell
  void setCell(const QString &value, int row, int column, int parentRow = 0,
               int parentColumn = 0);
  int getNumberOfRows();
  void clearTable();
  void setModel(QString const &name) override;
  void emitProcessClicked() override { emit processClicked(); }
  void emitProcessingFinished() override { emit processingFinished(); }
private:
  void createTable();
  void disableActionOnToolbar(int indexToDisable);
  void disableActionOnContextMenu(int indexToDisable);
  void enableActionOnToolbar(int indexToEnable);
  void enableActionOnContextMenu(int indexToEnable);

  static void disableActionOnWidget(QWidget &widget, int index);
  static void enableActionOnWidget(QWidget &widget, int index);
  static void disable(QWidget &widget);
  static void disable(QAction &widget);
  static void enable(QWidget &widget);
  static void enable(QAction &widget);

  // the presenter
  std::unique_ptr<DataProcessorPresenter> m_presenter;
  // the models
  boost::shared_ptr<AbstractDataProcessorTreeModel> m_model;
  // the interface
  Ui::DataProcessorWidget ui;
  // the workspace the user selected to open
  QString m_toOpen;
  // the context menu
  QMenu *m_contextMenu;
  QSignalMapper *m_openMap;
  // Command adapters
  std::vector<std::unique_ptr<DataProcessorCommandAdapter>> m_commands;

signals:
  void comboProcessInstrument_currentIndexChanged(int index);
  void ranPythonAlgorithm(const QString &pythonCode);
  void processButtonClicked();
  void processingFinished();
  void instrumentHasChanged();

public slots:
  void on_comboProcessInstrument_currentIndexChanged(int index);

private slots:
  void rowsUpdated(const QModelIndex &parent, int first, int last);
  void rowDataUpdated(const QModelIndex &topLeft,
                      const QModelIndex &bottomRight);
  void showContextMenu(const QPoint &pos);
  void processClicked();
  void ensureHasExtension(QString &filename) const;
};

} // namespace Mantid
} // namespace MantidWidgets

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_ */
