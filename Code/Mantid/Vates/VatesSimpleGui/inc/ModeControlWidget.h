#ifndef MODECONTROLWIDGET_H
#define MODECONTROLWIDGET_H

#include <QtGui/QWidget>
#include "ui_modecontrolwidget.h"

class ModeControlWidget : public QWidget
{
    Q_OBJECT

public:
    ModeControlWidget(QWidget *parent = 0);
    virtual ~ModeControlWidget();

    enum Views {STANDARD, THREESLICE, MULTISLICE};

signals:
	void executeSwitchViews(ModeControlWidget::Views v);

protected slots:
	void enableModeButtons();
	void onMultiSliceViewButtonClicked();
    void onStandardViewButtonClicked();
    void onThreeSliceViewButtonClicked();

private:
    Ui::ModeControlWidgetClass ui;
};

#endif // MODECONTROLWIDGET_H
