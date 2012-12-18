#ifndef PEAKSVIEWER_H
#define PEAKSVIEWER_H

#include <QtGui/QWidget>
#include "DllOption.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"

namespace MantidQt
{
namespace SliceViewer
{

class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksViewer : public QWidget
{
    Q_OBJECT
public:
    PeaksViewer(QWidget *parent = 0);
    void setPeaksWorkspaces(const SetPeaksWorkspaces& workspaces);
    ~PeaksViewer();
};

} //namespace
}
#endif // PEAKSVIEWER_H
