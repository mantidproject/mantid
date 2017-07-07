#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/ProgressableView.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"
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
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS QDataProcessorWidget
    : public MantidQt::API::MantidWidget,
      public DataProcessorView,
      public ProgressableView {

  Q_OBJECT
public:
  QDataProcessorWidget(std::unique_ptr<DataProcessorPresenter> presenter,
                       QWidget *parent = 0);
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

  // Add actions to the toolbar
  void addActions(
      std::vector<std::unique_ptr<DataProcessorCommand>> commands) override;

  // Connect the model
  void showTable(boost::shared_ptr<QAbstractItemModel> model) override;

  // Dialog/Prompt methods
  std::string requestNotebookPath() override;
  /// Dialog/Prompt methods
  std::string askUserString(const std::string &prompt, const std::string &title,
                            const std::string &defaultValue) override;
  bool askUserYesNo(std::string prompt, std::string title) override;
  void giveUserWarning(std::string prompt, std::string title) override;
  void giveUserCritical(std::string prompt, std::string title) override;
  std::string runPythonAlgorithm(const std::string &pythonCode) override;

  // Settings
  void saveSettings(const std::map<std::string, QVariant> &options) override;
  void loadSettings(std::map<std::string, QVariant> &options) override;

  // Set the status of the progress bar
  void setProgressRange(int min, int max) override;
  void setProgress(int progress) override;
  void clearProgress() override;

  // Get status of the checkbox which dictates whether an ipython notebook is
  // produced
  bool getEnableNotebook() override;

  // Expand/Collapse all groups
  void expandAll() override;
  void collapseAll() override;

  // Handle pause/resume of data reduction
  void pause() override;
  void resume() override;

  // Setter methods
  void setSelection(const std::set<int> &groups) override;
  void setTableList(const QSet<QString> &tables) override;
  void setSelectionModelConnections() override;
  void setInstrumentList(const QString &instruments,
                         const QString &defaultInstrument) override;
  void
  setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy *hintStrategy,
                         int column) override;
  void setClipboard(const std::string &text) override;

  // Transfer runs
  void transfer(const QList<QString> &runs);

  // Accessor methods
  std::map<int, std::set<int>> getSelectedChildren() const override;
  std::set<int> getSelectedParents() const override;
  std::string getProcessInstrument() const override;
  std::string getWorkspaceToOpen() const override;
  std::string getClipboard() const override;

  DataProcessorPresenter *getPresenter() const override;

  // Forward a main presenter to this view's presenter
  void accept(DataProcessorMainPresenter *);

private:
  // initialise the interface
  void createTable();
  // Set a selected model (table workspace)
  void setModel(const std::string &name) override;

  // the presenter
  std::unique_ptr<DataProcessorPresenter> m_presenter;
  // the models
  boost::shared_ptr<QAbstractItemModel> m_model;
  // the interface
  Ui::DataProcessorWidget ui;
  // the workspace the user selected to open
  std::string m_toOpen;
  // the context menu
  QMenu *m_contextMenu;
  QSignalMapper *m_openMap;
  // Command adapters
  std::vector<std::unique_ptr<DataProcessorCommandAdapter>> m_commands;

signals:
  void comboProcessInstrument_currentIndexChanged(int index);
  void runPythonAlgorithm(const QString &pythonCode);

public slots:
  void on_comboProcessInstrument_currentIndexChanged(int index);

private slots:
  void setModel(QString name);
  void tableUpdated(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void showContextMenu(const QPoint &pos);
  void processClicked();
  void newSelection(const QItemSelection &, const QItemSelection &);
};

} // namespace Mantid
} // namespace MantidWidgets

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_ */
