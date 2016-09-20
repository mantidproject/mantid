#ifndef MANTIDDOCK_H
#define MANTIDDOCK_H

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"

#include <QActionGroup>
#include <QAtomicInt>
#include <QComboBox>
#include <QDockWidget>
#include <QList>
#include <QPoint>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QMap>

#include <set>

class MantidUI;
class ApplicationWindow;
class MantidTreeWidgetItem;
class MantidTreeWidget;
class QLabel;
class QFileDialog;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;
class QSortFilterProxyModel;

enum MantidItemSortScheme { ByName, ByLastModified };

class MantidDockWidget : public QDockWidget {
  Q_OBJECT
public:
  MantidDockWidget(MantidUI *mui, ApplicationWindow *parent);
  ~MantidDockWidget() override;
  QString getSelectedWorkspaceName() const;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const;
  void dropEvent(QDropEvent *de) override;

public slots:
  void clickedWorkspace(QTreeWidgetItem *, int);
  void saveWorkspaceGroup();
  void deleteWorkspaces();
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
  void groupingButtonClick();
  void plotSpectra();
  void plotSpectraErr();
  void drawColorFillPlot();
  void showDetectorTable();
  void convertToMatrixWorkspace();
  void convertMDHistoToMatrixWorkspace();
  void updateTree();
  void incrementUpdateCount();
  void recordWorkspaceRename(QString, QString);
  void clearUB();
  void filterWorkspaceTree(const QString &text);
  void plotSurface();
  void plotContour();

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

protected:
  MantidTreeWidget *m_tree;
  friend class MantidUI;

private:
  QString selectedWsName;

  MantidUI *const m_mantidUI;

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
  Mantid::API::AnalysisDataServiceImpl &m_ads;
  /// Temporarily keeps names of selected workspaces during tree update
  /// in order to restore selection after update
  QStringList m_selectedNames;
  /// Keep a map of renamed workspaces between updates
  QMap<QString, QString> m_renameMap;
};
#endif
