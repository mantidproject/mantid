#ifndef PEAKSVIEWER_H
#define PEAKSVIEWER_H

#include <QtGui/QWidget>
#include "DllOption.h"
#include <boost/shared_ptr.hpp>
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"

namespace MantidQt
{
namespace SliceViewer
{
  /// Forward dec.
class ProxyCompositePeaksPresenter;

/**

*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksViewer : public QWidget
{
    Q_OBJECT
public:
    PeaksViewer(QWidget *parent = 0);
    void setPeaksWorkspaces(const SetPeaksWorkspaces& workspaces);
    void setPresenter(boost::shared_ptr<ProxyCompositePeaksPresenter> presenter);
    void hide();
    ~PeaksViewer();
public slots:
      void onPeakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
      void onBackgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor);
private:
  boost::shared_ptr<ProxyCompositePeaksPresenter> m_presenter;
};

} //namespace
}
#endif // PEAKSVIEWER_H
