// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_
#define MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_

#include "MantidKernel/System.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AbstractTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorView.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/HintStrategy.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include "MantidQtWidgets/Common/ProgressableView.h"
#include "ui_DataProcessorWidget.h"
#include <QSignalMapper>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class QtCommandAdapter;
class DataProcessorMainPresenter;
class PreprocessMap;
class ProcessingAlgorithm;
class PostprocessingAlgorithm;
class WhiteList;

/** QDataProcessorWidget : Provides an interface for processing table
data.
*/
class EXPORT_OPT_MANTIDQT_COMMON QDataProcessorWidget
    : public MantidQt::API::MantidWidget,
      public DataProcessorView,
      public ProgressableView {

  Q_OBJECT
public:
  QDataProcessorWidget(std::unique_ptr<DataProcessorPresenter> presenter,
                       QWidget *parent = nullptr);
  QDataProcessorWidget(const WhiteList & /*whitelist*/, QWidget *parent, int group = 0);
  QDataProcessorWidget(const WhiteList & /*whitelist*/, const ProcessingAlgorithm & /*algorithm*/,
                       QWidget *parent, int group = 0);
  QDataProcessorWidget(const WhiteList & /*whitelist*/, const PreprocessMap & /*preprocessMap*/,
                       const ProcessingAlgorithm & /*algorithm*/, QWidget *parent,
                       int group = 0);
  QDataProcessorWidget(const WhiteList & /*whitelist*/, const ProcessingAlgorithm & /*algorithm*/,
                       const PostprocessingAlgorithm & /*postprocessor*/, QWidget *parent,
                       int group = 0);
  QDataProcessorWidget(const WhiteList & /*whitelist*/, const PreprocessMap & /*preprocessMap*/,
                       const ProcessingAlgorithm & /*algorithm*/,
                       const PostprocessingAlgorithm & /*postprocessor*/, QWidget *parent,
                       int group = 0);
  ~QDataProcessorWidget() override;

  // Add actions to the toolbar
  void addActions(std::vector<std::unique_ptr<Command>> commands) override;

  // Connect the model
  void showTable(boost::shared_ptr<AbstractTreeModel> model) override;

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
  bool getEnableNotebook() override;

  // Expand/Collapse all groups
  void expandAll() override;
  void collapseAll() override;

  // Select all rows/groups
  void selectAll() override;

  // Update enabled/disabled state of widgets
  void updateMenuEnabledState(const bool isProcessing) override;
  void setProcessButtonEnabled(const bool enabled) override;
  void setInstrumentComboEnabled(const bool enabled) override;
  void setTreeEnabled(const bool enabled) override;
  void setOutputNotebookEnabled(const bool enabled) override;

  // Setter methods
  void setSelection(const std::set<int> &groups) override;
  void setInstrumentList(const QString &instruments,
                         const QString &defaultInstrument) override;
  void
  setOptionsHintStrategy(MantidQt::MantidWidgets::HintStrategy *hintStrategy,
                         int column) override;
  void setClipboard(const QString &text) override;
  void setItemDelegate() override;

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
  void accept(DataProcessorMainPresenter * /*mainPresenter*/);

  // Force re-processing of rows
  void setForcedReProcessing(bool forceReProcessing) override;

  // Get value in a cell
  QString getCell(int row, int column, int parentRow = 0, int parentColumn = 0);
  // Set value in a cell
  void setCell(const QString &value, int row, int column, int parentRow = 0,
               int parentColumn = 0);
  int getNumberOfRows();
  void clearTable();

  // Methods to emit signals
  void emitProcessClicked() override { emit processButtonClicked(); };

  void emitProcessingFinished() override { emit processingFinished(); }

  void skipProcessing() override;

  void enableGrouping() override;
  void disableGrouping() override;

  void settingsChanged();
signals:
  void processButtonClicked();
  void processingFinished();
  void instrumentHasChanged();
  void dataChanged(const QModelIndex & /*_t1*/, const QModelIndex & /*_t2*/);

private:
  // initialise the interface
  void createTable();

  // the presenter
  std::unique_ptr<DataProcessorPresenter> m_presenter;
  // the models
  boost::shared_ptr<AbstractTreeModel> m_model;
  // Command adapters
  std::vector<std::unique_ptr<QtCommandAdapter>> m_commands;
  // the interface (uses actions owned by m_commands)
  Ui::DataProcessorWidget ui;
  // the workspace the user selected to open
  QString m_toOpen;
  // the context menu
  QMenu *m_contextMenu;
  QSignalMapper *m_openMap;

signals:
  void comboProcessInstrument_currentIndexChanged(int index);
  void ranPythonAlgorithm(const QString &pythonCode);

public slots:
  void on_comboProcessInstrument_currentIndexChanged(int index);
  void setModel(QString const &name) override;

private slots:
  void rowsUpdated(const QModelIndex &parent, int first, int last);
  void rowDataUpdated(const QModelIndex &topLeft,
                      const QModelIndex &bottomRight);
  void showContextMenu(const QPoint &pos);
  void processClicked();
  void ensureHasExtension(QString &filename) const;
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /* MANTIDQTMANTIDWIDGETS_QDATAPROCESSORWIDGET_H_ */
