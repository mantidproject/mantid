#ifndef LINEVIEWER_H
#define LINEVIEWER_H

#include <QtGui/QWidget>
#include "ui_LineViewer.h"

namespace MantidQt
{
namespace SliceViewer
{

class LineViewer : public QWidget
{
    Q_OBJECT

public:
    LineViewer(QWidget *parent = 0);
    ~LineViewer();

private:
    Ui::LineViewerClass ui;
};

} //namespace
}
#endif // LINEVIEWER_H
