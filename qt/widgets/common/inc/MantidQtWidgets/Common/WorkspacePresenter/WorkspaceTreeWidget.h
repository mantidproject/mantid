// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/MantidTreeWidget.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include "MantidQtWidgets/Common/WorkspacePresenter/IWorkspaceDockView.h"
#include <QDockWidget>
#include <QHash>
#include <QMap>
#include <QMetaType>
#include <QMutex>
#include <map>
#include <memory>

class QMainWindow;
class QLabel;
class QFileDialog;
class QLineEdit;
class QActionGroup;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;
class QSortFilterProxyModel;

using TopLevelItems = std::map<std::string, Mantid::API::Workspace_sptr>;
Q_DECLARE_METATYPE(TopLevelItems)

namespace MantidQt {
namespace MantidWidgets {
class MantidDisplayBase;
class MantidTreeWidgetItem;
class MantidTreeWidget;

/**
\class  WorkspaceTreeWidget
\author Lamar Moore
\date   24-08-2016
\version 1.1
*/
class EXPORT_OPT_MANTIDQT_COMMON WorkspaceTreeWidget : public QWidget, public IWorkspaceDockView {
  Q_OBJECT
public:
  WorkspaceTreeWidget(MantidQt::MantidWidgets::MantidDisplayBase *mdb, bool viewOnly = false,
                      QWidget *parent = nullptr);
  ~WorkspaceTreeWidget();
  void dropEvent(QDropEvent *de) override;

  MantidQt::MantidWidgets::WorkspacePresenterWN_wptr getPresenterWeakPtr() override;

  SortDirection getSortDirection() const override;
  SortCriteria getSortCriteria() const override;
  void sortWorkspaces(SortCriteria criteria, SortDirection direction) override;

  MantidQt::MantidWidgets::StringList getSelectedWorkspaceNames() const override;
  // Horrible second function to get a return value as QStringList directly
  QStringList getSelectedWorkspaceNamesAsQList() const;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const override;

  bool askUserYesNo(const std::string &caption, const std::string &message) const override;
  void showCriticalUserMessage(const std::string &caption, const std::string &message) const override;

  void showLoadDialog() override;
  void showLiveDataDialog() override;
  void showRenameDialog(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void enableDeletePrompt(bool enable) override;
  bool isPromptDelete() const override;
  bool deleteConfirmation() const override;
  void deleteWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  bool clearWorkspacesConfirmation() const override;
  void enableClearButton(bool enable) override;
  void clearView() override;
  std::string getFilterText() const override;
  SaveFileType getSaveFileType() const override;
  void saveWorkspace(const std::string &wsName, SaveFileType type) override;
  void saveWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void filterWorkspaces(const std::string &filterText) override;
  void recordWorkspaceRename(const std::string &oldName, const std::string &newName) override;
  void refreshWorkspaces() override;

  // Context Menu Handlers
  void popupContextMenu() override;
  void showWorkspaceData() override;
  void saveToProgram() override;
  void showInstrumentView() override;
  void plotSpectrum(const std::string &type) override;
  void showColourFillPlot() override;
  void showDetectorsTable() override;
  void showBoxDataTable() override;
  void showMDPlot() override;
  void showListData() override;
  void showSpectrumViewer() override;
  void showSliceViewer() override;
  void showLogs() override;
  void showSampleMaterialWindow() override;
  void showAlgorithmHistory() override;
  void showTransposed() override;
  void convertToMatrixWorkspace() override;
  void convertMDHistoToMatrixWorkspace() override;

  bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg, const bool wait = true) override;

private:
  bool hasUBMatrix(const std::string &wsName);
  void addSaveMenuOption(const QString &algorithmString, QString menuEntryName = "");
  void setTreeUpdating(const bool state);
  inline bool isTreeUpdating() const { return m_treeUpdating; }
  void updateTree(const TopLevelItems &items) override;
  void populateTopLevel(const TopLevelItems &topLevelItems, const QStringList &expanded);
  MantidTreeWidgetItem *addTreeEntry(const std::pair<std::string, Mantid::API::Workspace_sptr> &item,
                                     QTreeWidgetItem *parent = nullptr);
  bool shouldBeSelected(const QString &name) const;
  void createWorkspaceMenuActions();
  void createSortMenuActions();
  void setItemIcon(QTreeWidgetItem *item, const std::string &wsID);

  void addMatrixWorkspaceMenuItems(QMenu *menu, const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const;
  void addMDEventWorkspaceMenuItems(QMenu *menu, const Mantid::API::IMDEventWorkspace_const_sptr &mdeventWS) const;
  void addMDHistoWorkspaceMenuItems(QMenu *menu, const Mantid::API::IMDWorkspace_const_sptr &WS) const;
  void addPeaksWorkspaceMenuItems(QMenu *menu, const Mantid::API::IPeaksWorkspace_const_sptr &WS) const;
  void addWorkspaceGroupMenuItems(QMenu *menu) const;
  void addTableWorkspaceMenuItems(QMenu *menu) const;
  void addClearMenuItems(QMenu *menu, const QString &wsName);

  void excludeItemFromSort(MantidTreeWidgetItem *item);

  void setupWidgetLayout();
  void setupLoadButtonMenu();
  void setupConnections();
  void hideButtonToolbar();

  MantidQt::MantidWidgets::MantidItemSortScheme whichCriteria(SortCriteria criteria);

public slots:
  void clickedWorkspace(QTreeWidgetItem * /*item*/, int /*unused*/);
  void saveWorkspaceCollection();
  void onClickDeleteWorkspaces();
  void onClickClearWorkspaces();
  void renameWorkspace();
  void populateChildData(QTreeWidgetItem *item);
  void onClickSaveToProgram(const QString &name);
  void sortAscending();
  void sortDescending();
  void chooseByName();
  void chooseByLastModified();
  void chooseByMemorySize();
  void keyPressEvent(QKeyEvent * /*unused*/) override;

protected slots:
  void popupMenu(const QPoint &pos);
  void workspaceSelected();

private slots:
  void handleShowSaveAlgorithm();
  void onTreeSelectionChanged();
  void onClickGroupButton();
  void onClickLoad();
  void onLoadAccept();
  void onClickLiveData();
  void onClickShowData();
  void onClickShowInstrument();
  void onClickShowBoxData();
  void onClickShowMDPlot();
  void onClickShowListData();
  void onClickShowSpectrumViewer();
  void onClickShowSliceViewer();
  void onClickShowFileLog();
  void onClickSaveNexusWorkspace();
  void onClickShowTransposed();
  void onClickPlotSpectra();
  void onClickPlotSpectraErr();
  void onClickPlotAdvanced();
  void onClickDrawColorFillPlot();
  void onClickShowDetectorTable();
  void onClickConvertToMatrixWorkspace();
  void onClickConvertMDHistoToMatrixWorkspace();
  void onClickShowAlgHistory();
  void onClickShowSampleMaterial();
  void onClickClearUB();
  void incrementUpdateCount();
  void filterWorkspaceTree(const QString &text);

private:
  MantidQt::MantidWidgets::WorkspacePresenterVN_sptr m_presenter;

protected:
  MantidTreeWidget *m_tree;
  QPoint m_menuPosition;
  QString selectedWsName;
  QMenu *m_loadMenu, *m_saveToProgram;
  QSignalMapper *m_programMapper;
  QAction *m_program, *m_saveNexus, *m_rename, *m_delete;

private:
  QString m_programName;
  MantidDisplayBase *const m_mantidDisplayModel;

  std::string m_filteredText;
  QPushButton *m_loadButton;
  QPushButton *m_saveButton;
  QPushButton *m_deleteButton;
  QPushButton *m_clearButton;
  QPushButton *m_groupButton;
  QPushButton *m_sortButton;
  QLineEdit *m_workspaceFilter;
  QFileDialog *m_saveFolderDialog;
  bool m_viewOnly;

  QMenu *m_saveMenu;
  // Context-menu actions
  QAction *m_showData, *m_showInst, *m_plotSpec, *m_plotSpecErr, *m_plotAdvanced, *m_showDetectors, *m_showBoxData,
      *m_showSpectrumViewer, *m_showSliceViewer, *m_colorFill, *m_showLogs, *m_showSampleMaterial, *m_showHist,
      *m_showMDPlot, *m_showListData, *m_showTransposed, *m_convertToMatrixWorkspace,
      *m_convertMDHistoToMatrixWorkspace, *m_clearUB;

  QAtomicInt m_updateCount;
  bool m_treeUpdating;
  bool m_promptDelete;
  SaveFileType m_saveFileType;
  SortCriteria m_sortCriteria;
  SortDirection m_sortDirection;
  /// Temporarily keeps names of selected workspaces during tree update
  /// in order to restore selection after update
  QStringList m_selectedNames;
  /// Keep a map of renamed workspaces between updates
  QHash<QString, QString> m_renameMap;
  /// A mutex to lock m_renameMap and m_selectedNames for reading/writing
  mutable QMutex m_mutex;

private slots:
  void handleUpdateTree(const TopLevelItems & /*items*/);
  void handleClearView();
signals:
  void signalClearView();
  void signalUpdateTree(const TopLevelItems & /*_t1*/);
};
} // namespace MantidWidgets
} // namespace MantidQt
