#ifndef SLICEVIEWER_H
#define SLICEVIEWER_H

#include <QtGui/QWidget>
#include "ui_SliceViewer.h"

class SliceViewer : public QWidget
{
    Q_OBJECT

public:
    SliceViewer(QWidget *parent = 0);
    ~SliceViewer();

private:
    Ui::SliceViewerClass ui;
};

#endif // SLICEVIEWER_H
