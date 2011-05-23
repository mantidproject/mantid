#ifndef MPMAINWINDOW_H
#define MPMAINWINDOW_H

#include <QtGui/QMainWindow>
#include "ui_MpMainWindow.h"
#include <QPointer>

class IView;

class pqPipelineSource;

class mpMainWindow : public QMainWindow, public Ui::mpMainWindow
{
    Q_OBJECT

public:
    mpMainWindow(QWidget *parent = 0);
    virtual ~mpMainWindow();

protected slots:
    void onDataLoaded(pqPipelineSource *);
    void switchViews(ModeControlWidget::Views v);

signals:
	void disableViewModes();
	void enableModeButtons();

private:
    Q_DISABLE_COPY(mpMainWindow);
    QPointer<pqPipelineSource> originSource;
    IView *currentView;
    IView *hiddenView;

    void removeProxyTabWidgetConnections();
    void setMainWindowComponentsForView();
    IView *setMainViewWidget(QWidget *container, ModeControlWidget::Views v);
    void swapViews();
};

#endif // mpMAINWINDOW_H
