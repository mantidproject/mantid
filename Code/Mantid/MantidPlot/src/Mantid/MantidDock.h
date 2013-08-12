#ifndef MANTIDDOCK_H
#define MANTIDDOCK_H

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtMantidWidgets/AlgorithmSelectorWidget.h"

#include <QActionGroup>
#include <QAtomicInt>
#include <QComboBox>
#include <QDockWidget>
#include <QList>
#include <QPoint>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSortFilterProxyModel>
#include <QStringList>

#include <set>

class MantidUI;
class ApplicationWindow;
class MantidTreeWidgetItem;
class MantidTreeWidget;
class QLabel;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressBar;
class QVBoxLayout;
class QHBoxLayout;
class QSignalMapper;
class QSortFilterProxyModel;

enum MantidItemSortScheme
{
  ByName, ByLastModified
};

class MantidDockWidget: public QDockWidget
{
  Q_OBJECT
public:
  MantidDockWidget(MantidUI *mui, ApplicationWindow *parent);
  ~MantidDockWidget();
  QString getSelectedWorkspaceName() const;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const;

public slots:
  void clickedWorkspace(QTreeWidgetItem*, int);
  void deleteWorkspaces();
  void renameWorkspace();
  void populateChildData(QTreeWidgetItem* item);
  void saveToProgram(const QString & name);
  void sortAscending();
  void sortDescending();
  void chooseByName();
  void chooseByLastModified();
protected slots:
  void popupMenu(const QPoint & pos);
  void workspaceSelected();

private slots:
  QTreeWidgetItem *addWorkspaceTreeEntry(Mantid::API::Workspace::InfoNode &node, QTreeWidgetItem* parentItem = NULL);
  void treeSelectionChanged();
  void groupingButtonClick();
  void plotSpectra();
  void plotSpectraDistribution();
  void plotSpectraErr();
  void plotSpectraDistributionErr();
  void drawColorFillPlot();
  void showDetectorTable();
  void convertToMatrixWorkspace();
  void convertMDHistoToMatrixWorkspace();
  void updateTree();
  void incrementUpdateCount();

private:
  void setTreeUpdating(const bool state);
  inline bool isTreeUpdating() const { return m_treeUpdating; }
  void populateTopLevel(const std::map<std::string,Mantid::API::Workspace_sptr> & topLevelItems, const QStringList & expanded);
  MantidTreeWidgetItem * addTreeEntry(const std::pair<std::string,Mantid::API::Workspace_sptr> & item, QTreeWidgetItem* parent = NULL);
  void createWorkspaceMenuActions();
  void createSortMenuActions();
  QString findParentName(const QString & ws_name, Mantid::API::Workspace_sptr workspace);
  void setItemIcon(QTreeWidgetItem *item,  const std::string & wsID);

  void addMatrixWorkspaceMenuItems(QMenu *menu, Mantid::API::MatrixWorkspace_const_sptr matrixWS) const;
  void addMDEventWorkspaceMenuItems(QMenu *menu, Mantid::API::IMDEventWorkspace_const_sptr mdeventWS) const;
  void addMDHistoWorkspaceMenuItems(QMenu *menu, Mantid::API::IMDWorkspace_const_sptr WS) const;
  void addPeaksWorkspaceMenuItems(QMenu *menu, Mantid::API::IPeaksWorkspace_const_sptr WS) const;
  void addWorkspaceGroupMenuItems(QMenu *menu) const;
  void addTableWorkspaceMenuItems(QMenu * menu) const;

  void excludeItemFromSort(MantidTreeWidgetItem *item);
  
protected:
  MantidTreeWidget * m_tree;
  friend class MantidUI;

private:
  QString selectedWsName;
  
  MantidUI * const m_mantidUI;
  QSet<QString> m_known_groups;

  QPushButton *m_loadButton;
  QMenu *m_loadMenu, *m_saveToProgram, *m_sortMenu;
  QPushButton *m_deleteButton;
  QPushButton *m_groupButton;
  QPushButton *m_sortButton;
  QSignalMapper *m_loadMapper, *m_programMapper;
  QActionGroup *m_sortChoiceGroup;

  //Context-menu actions
  QAction *m_showData, *m_showInst, *m_plotSpec, *m_plotSpecErr, *m_plotSpecDistr,
  *m_showDetectors, *m_showBoxData, *m_showVatesGui,
  *m_showImageViewer,
  *m_showSliceViewer,
  *m_colorFill, *m_showLogs, *m_showHist, *m_showMDPlot, *m_showListData,
  *m_saveNexus, *m_rename, *m_delete,
  *m_program, * m_ascendingSortAction,
  *m_descendingSortAction, *m_byNameChoice, *m_byLastModifiedChoice, *m_showTransposed,
  *m_convertToMatrixWorkspace,
  *m_convertMDHistoToMatrixWorkspace;

  QAtomicInt m_updateCount;
  bool m_treeUpdating;
  Mantid::API::AnalysisDataServiceImpl & m_ads;
  Mantid::API::Workspace::InfoNode *m_rootInfoNode;

  static Mantid::Kernel::Logger& logObject;
};

class MantidTreeWidget:public QTreeWidget
{
  Q_OBJECT

public:
  MantidTreeWidget(MantidDockWidget *w, MantidUI *mui);
  void mousePressEvent (QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void mouseDoubleClickEvent(QMouseEvent *e);

  QStringList getSelectedWorkspaceNames() const;
  QMultiMap<QString,std::set<int> > chooseSpectrumFromSelected() const;
  void setSortScheme(MantidItemSortScheme);
  void setSortOrder(Qt::SortOrder);
  MantidItemSortScheme getSortScheme() const;
  Qt::SortOrder getSortOrder() const;
  void logWarningMessage(const std::string&);
  void disableNodes(bool);
  void sort();

private:
  QPoint m_dragStartPosition;
  MantidDockWidget *m_dockWidget;
  MantidUI *m_mantidUI;
  Mantid::API::AnalysisDataServiceImpl & m_ads;
  static Mantid::Kernel::Logger& logObject;
  MantidItemSortScheme m_sortScheme;
  Qt::SortOrder m_sortOrder;
};

/**A class derived from QTreeWidgetItem, to accomodate
 * sorting on the items in a MantidTreeWidget.
 */
class MantidTreeWidgetItem : public QTreeWidgetItem
{
public:
  MantidTreeWidgetItem(MantidTreeWidget*);
  MantidTreeWidgetItem(QStringList, MantidTreeWidget*);
  void disableIfNode(bool);
  void setSortPos(int o) {m_sortPos = o;}
  int getSortPos() const {return m_sortPos;}

private:
  bool operator<(const QTreeWidgetItem &other) const;
  MantidTreeWidget* m_parent;
  static Mantid::Kernel::DateAndTime getLastModified(const QTreeWidgetItem*);
  int m_sortPos;
};


class AlgorithmDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w);
public slots:
    void update();
    void updateProgress(void* alg, const double p, const QString& msg, double estimatedTime, int progressPrecision);
    void algorithmStarted(void* alg);
    void algorithmFinished(void* alg);
protected:
    void showProgressBar();
    void hideProgressBar();

    MantidQt::MantidWidgets::AlgorithmSelectorWidget * m_selector;
    QPushButton *m_runningButton;
    QProgressBar* m_progressBar;
    QHBoxLayout * m_runningLayout;
    QList<void*> m_algID;
    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
};


#endif
