#ifndef MANTIDDOCK_H
#define MANTIDDOCK_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QPoint>
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

class MantidUI;
class ApplicationWindow;
class MantidTreeWidget;
class QLabel;
class QMenu;
class QPushButton;


class MantidDockWidget: public QDockWidget
{
  Q_OBJECT
public:
  MantidDockWidget(MantidUI *mui, ApplicationWindow *parent);
  QString getSelectedWorkspaceName() const;
  Mantid::API::Workspace_sptr getSelectedWorkspace() const;
private:
  bool isItWorkspaceGroupItem(Mantid::API::WorkspaceGroup_sptr grpSPtr,const QString & ws_name);
  bool isItWorkspaceGroupParentItem(Mantid::API::Workspace_sptr workspace);
  void removeFromWSGroupNames(const QString& wsName);
  Mantid::API::WorkspaceGroup_sptr getGroupWorkspace(QTreeWidgetItem *parentItem);
  bool isTheWorkspaceToRenameIsaGroupMember(const QString & wsName,Mantid::API::WorkspaceGroup_sptr grpwsSptr );
  QTreeWidgetItem* getparentWorkspaceItem(const QString & wsName);
public slots:
  void clickedWorkspace(QTreeWidgetItem*, int);
  void deleteWorkspaces();
  void renameWorkspace();
protected slots:
  void popupMenu(const QPoint & pos);
  void workspaceSelected();
private slots:
  void populateWorkspaceData(Mantid::API::Workspace_sptr workspace, QTreeWidgetItem* ws_item);
  void updateWorkspaceEntry(const QString &, Mantid::API::Workspace_sptr);
  void updateWorkspaceTreeafterRenaming(const QString &, const QString &);
  void workspaceGroupRemoved(const QString &);
  void removeWorkspaceEntry(const QString &);
  void populateWorkspaceTree(const QString & ws_name, Mantid::API::Workspace_sptr,bool isitParent);
  void treeSelectionChanged();
  void groupOrungroupWorkspaces();
  void plotSpectra();

protected:
  MantidTreeWidget * m_tree;
  friend class MantidUI;
private:
  MantidUI * const m_mantidUI;
  QPushButton *m_loadButton;
  QPushButton *m_deleteButton;
  QPushButton *m_groupButton;

  static Mantid::Kernel::Logger& logObject;

};


class MantidTreeWidget:public QTreeWidget
{
  Q_OBJECT

public:
  MantidTreeWidget(QWidget *w, MantidUI *mui);
  void mousePressEvent (QMouseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void mouseDoubleClickEvent(QMouseEvent *e);

  QList<QString> getSelectedWorkspaceNames() const;
  QMultiMap<QString,int> chooseSpectrumFromSelected() const;

private:
  QPoint m_dragStartPosition;
  MantidUI *m_mantidUI;
  static Mantid::Kernel::Logger& logObject;
};

class FindAlgComboBox:public QComboBox
{
    Q_OBJECT
signals:
    void enterPressed();
protected:
    void keyPressEvent(QKeyEvent *e);
};

class AlgorithmDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w);
public slots:
    void update();
    void findAlgTextChanged(const QString& text);
    void treeSelectionChanged();
    void selectionChanged(const QString& algName);
    void countChanged(int n);
    void tst();
protected:
    QTreeWidget *m_tree;
    FindAlgComboBox* m_findAlg;
    QLabel *m_runningAlgsLabel;
    bool m_treeChanged;
    bool m_findAlgChanged;
    friend class MantidUI;
private:
    MantidUI *m_mantidUI;
};


class AlgorithmTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    AlgorithmTreeWidget(QWidget *w, MantidUI *mui):QTreeWidget(w),m_mantidUI(mui){}
    void mousePressEvent (QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
private:
    QPoint m_dragStartPosition;
    MantidUI *m_mantidUI;
};

#endif
