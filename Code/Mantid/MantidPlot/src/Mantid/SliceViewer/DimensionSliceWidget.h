#ifndef DIMENSIONSLICEWIDGET_H
#define DIMENSIONSLICEWIDGET_H

#include <QtGui/QWidget>
#include "ui_DimensionSliceWidget.h"

class DimensionSliceWidget : public QWidget
{
    Q_OBJECT

public:
    DimensionSliceWidget(QWidget *parent = 0);
    ~DimensionSliceWidget();

private:
    Ui::DimensionSliceWidgetClass ui;
};

#endif // DIMENSIONSLICEWIDGET_H
