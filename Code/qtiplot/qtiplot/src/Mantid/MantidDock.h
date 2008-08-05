#ifndef MANTIDDOCK_H
#define MANTIDDOCK_H

#include <QDockWidget>
#include <QTreeWidget>
#include <QPushButton>

class MantidUI;
class ApplicationWindow;

class MantidDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    MantidDockWidget(QWidget*w):QDockWidget(w){}
    MantidDockWidget(MantidUI *mui, ApplicationWindow *w);
    void update();
public slots:
    void clickedWorkspace(QTreeWidgetItem*, int);
protected:
    QTreeWidget *m_tree;
    friend class MantidUI;
private:
    QPushButton *m_loadButton;
    QPushButton *m_deleteButton;
    MantidUI *m_mantidUI;
};


class MantidTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    MantidTreeWidget(QWidget *w):QTreeWidget(w){}
    void mousePressEvent (QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
private:
    QPoint m_dragStartPosition;
};

class AlgorithmDockWidget: public QDockWidget
{
    Q_OBJECT
public:
    AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w);
public slots:
    void update();
    void tst();
protected:
    QTreeWidget *m_tree;
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
