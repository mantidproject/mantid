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
  ~QDataProcessorWidget() override;

  // Add actions to the toolbar
  void addActions(
      std::vector<std::unique_ptr<DataProcessorCommand>> commands) override;

  // Connect the model
  void showTable(boost::shared_ptr<QAbstractItemModel> model) override;

  // Dialog/Prompt methods
  std::string requestNotebookPath() override;

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

  // Settor methods
  void setSelection(const std::set<int> &groups) override;
  void setTableList(const std::set<std::string> &tables) override;
  void setInstrumentList(const std::vector<std::string> &instruments,
                         const std::string &defaultInstrument) override;
  void
  setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy *hintStrategy,
                         int column) override;
  void setClipboard(const std::string &text) override;

  // Accessor methods
  std::map<int, std::set<int>> getSelectedChildren() const override;
  std::set<int> getSelectedParents() const override;
  std::string getProcessInstrument() const override;
  std::string getWorkspaceToOpen() const override;
  std::string getClipboard() const override;

  DataProcessorPresenter *getPresenter() const override;

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
  QSignalMapper *m_openMap;
  // Command adapters
  std::vector<std::unique_ptr<DataProcessorCommandAdapter>> m_commands;

signals:
  void comboProcessInstrument_currentIndexChanged(int index);

public slots:
  void on_comboProcessInstrument_currentIndexChanged(int index);

private slots:
  void setModel(QString name);
  void tableUpdated(const QModelIndex &topLeft, const QModelIndex &bottomRight);
  void showContextMenu(const QPoint &pos);
  void processClicked();
};

} // namespace Mantid
} // namespace MantidWidgets

#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_ */
