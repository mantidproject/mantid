#ifndef QWORKSPACEDOCKVIEW_H
#define QWORKSPACEDOCKVIEW_H

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include <MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h>
#include <QDockWidget>
#include <QMap>
#include <boost/shared_ptr.hpp>

class MantidUI;
class ApplicationWindow;
class MantidTreeWidgetItem;
class MantidTreeWidget;
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

/**
\class  QWorkspaceDockView
\author Lamar Moore
\date   24-08-2016
\version 1.0


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
*/
class QWorkspaceDockView : public MantidQt::MantidWidgets::IWorkspaceDockView,
                           public QDockWidget {
  Q_OBJECT
public:
  explicit QWorkspaceDockView(MantidUI *mui, ApplicationWindow *parent);
  ~QWorkspaceDockView() override;

  void init() override;
  MantidQt::MantidWidgets::WorkspacePresenterWN_wptr
  getPresenterWeakPtr() override;

  SortDirection getSortDirection() const override;
  SortCriteria getSortCriteria() const override;
  void sortWorkspaces(SortCriteria criteria, SortDirection direction) override;

  MantidQt::MantidWidgets::StringList
  getSelectedWorkspaceNames() const override;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const override;

  bool askUserYesNo(const std::string &caption,
                    const std::string &message) const override;
  void showCriticalUserMessage(const std::string &caption,
                               const std::string &message) const override;

  void showLoadDialog() override;
  void showLiveDataDialog() override;
  void showRenameDialog(
      const MantidQt::MantidWidgets::StringList &wsNames) const override;
  void groupWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames,
                       const std::string &groupName) const override;
  void ungroupWorkspaces(
      const MantidQt::MantidWidgets::StringList &wsNames) const override;
  void enableDeletePrompt(bool enable) override;
  bool isPromptDelete() const override;
  bool deleteConfirmation() const override;
  void
  deleteWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void clearView() override;
  SaveFileType getSaveFileType() const override;
  void saveWorkspace(const std::string &wsName, SaveFileType type) override;
  void
  saveWorkspaces(const MantidQt::MantidWidgets::StringList &wsNames) override;
  void updateTree(
      const std::map<std::string, Mantid::API::Workspace_sptr> &items) override;
  std::string getFilterText() const override;
  void filterWorkspaces(const std::string &filterText) override;
  void recordWorkspaceRename(const std::string &oldName,
                             const std::string &newName) override;

private:
  void addSaveMenuOption(QString algorithmString, QString menuEntryName = "");
  void setTreeUpdating(const bool state);
  inline bool isTreeUpdating() const { return m_treeUpdating; }
  void populateTopLevel(
      const std::map<std::string, Mantid::API::Workspace_sptr> &topLevelItems,
      const QStringList &expanded);
  MantidTreeWidgetItem *
  addTreeEntry(const std::pair<std::string, Mantid::API::Workspace_sptr> &item,
               QTreeWidgetItem *parent = NULL);
  bool shouldBeSelected(QString name) const;
  void createWorkspaceMenuActions();
  void createSortMenuActions();
  void setItemIcon(QTreeWidgetItem *item, const std::string &wsID);

  void addMatrixWorkspaceMenuItems(
      QMenu *menu,
      const Mantid::API::MatrixWorkspace_const_sptr &matrixWS) const;
  void addMDEventWorkspaceMenuItems(
      QMenu *menu,
      const Mantid::API::IMDEventWorkspace_const_sptr &mdeventWS) const;
  void addMDHistoWorkspaceMenuItems(
      QMenu *menu, const Mantid::API::IMDWorkspace_const_sptr &WS) const;
  void addPeaksWorkspaceMenuItems(
      QMenu *menu, const Mantid::API::IPeaksWorkspace_const_sptr &WS) const;
  void addWorkspaceGroupMenuItems(
      QMenu *menu, const Mantid::API::WorkspaceGroup_const_sptr &groupWS) const;
  void addTableWorkspaceMenuItems(QMenu *menu) const;
  void addClearMenuItems(QMenu *menu, const QString &wsName);

  void excludeItemFromSort(MantidTreeWidgetItem *item);

  void setupWidgetLayout();
  void setupLoadButtonMenu();
  void setupConnections();

  void showAlgorithm(const std::string &algName);
  Mantid::API::IAlgorithm_sptr createAlgorithm(const std::string &algName);

public slots:
  void clickedWorkspace(QTreeWidgetItem *, int);
  void saveWorkspaceGroup();
  void onClickDeleteWorkspaces();
  void renameWorkspace();
  void populateChildData(QTreeWidgetItem *item);
  void saveToProgram(const QString &name);
  void sortAscending();
  void sortDescending();
  void chooseByName();
  void chooseByLastModified();
  void saveWorkspacesToFolder(const QString &folder);

protected slots:
  void popupMenu(const QPoint &pos);
  void workspaceSelected();

private slots:
  void handleShowSaveAlgorithm();
  void treeSelectionChanged();
  void onClickGroupButton();
  void onClickLoad();
  void onLoadAccept();
  void onClickLiveData();
  void plotSpectra();
  void plotSpectraErr();
  void drawColorFillPlot();
  void showDetectorTable();
  void convertToMatrixWorkspace();
  void convertMDHistoToMatrixWorkspace();
  void updateTree();
  void incrementUpdateCount();
  void clearUB();
  void filterWorkspaceTree(const QString &text);
  void plotSurface();
  void plotContour();

private:
  MantidQt::MantidWidgets::WorkspacePresenterVN_sptr m_presenter;

protected:
  MantidTreeWidget *m_tree;
  friend class MantidUI;

private:
  QString selectedWsName;

  MantidUI *const m_mantidUI;

  std::string m_filteredText;
  QPushButton *m_loadButton;
  QPushButton *m_saveButton;
  QMenu *m_loadMenu, *m_saveToProgram, *m_sortMenu, *m_saveMenu;
  QPushButton *m_deleteButton;
  QPushButton *m_groupButton;
  QPushButton *m_sortButton;
  QLineEdit *m_workspaceFilter;
  QSignalMapper *m_loadMapper, *m_programMapper;
  QActionGroup *m_sortChoiceGroup;
  QFileDialog *m_saveFolderDialog;

  // Context-menu actions
  QAction *m_showData, *m_showInst, *m_plotSpec, *m_plotSpecErr,
      *m_showDetectors, *m_showBoxData, *m_showVatesGui, *m_showSpectrumViewer,
      *m_showSliceViewer, *m_colorFill, *m_showLogs, *m_showSampleMaterial,
      *m_showHist, *m_showMDPlot, *m_showListData, *m_saveNexus, *m_rename,
      *m_delete, *m_program, *m_ascendingSortAction, *m_descendingSortAction,
      *m_byNameChoice, *m_byLastModifiedChoice, *m_showTransposed,
      *m_convertToMatrixWorkspace, *m_convertMDHistoToMatrixWorkspace,
      *m_clearUB, *m_plotSurface, *m_plotContour;

  ApplicationWindow *m_appParent;

  QAtomicInt m_updateCount;
  bool m_treeUpdating;
  bool m_promptDelete;
  /// Temporarily keeps names of selected workspaces during tree update
  /// in order to restore selection after update
  QStringList m_selectedNames;
  /// Keep a map of renamed workspaces between updates
  QHash<QString, QString> m_renameMap;

signals:
  void updateRecentFiles(const QString &);
};
#endif // QWORKSPACEDOCKVIEW_H