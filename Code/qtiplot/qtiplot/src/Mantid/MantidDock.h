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
    //void mousePressEvent ( QMouseEvent * event );
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

#endif
