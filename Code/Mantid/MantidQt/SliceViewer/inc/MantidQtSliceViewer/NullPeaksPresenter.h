#ifndef MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /*---------------------------------------------------------
    NullPeaksPresenter

    This implementation prevents the client code having to run Null checks on the PeaksPresenter pointer before using it.
    ----------------------------------------------------------*/
    class DLLExport NullPeaksPresenter : public PeaksPresenter
    {
    public:
      virtual void update(){}
      virtual void updateWithSlicePoint(const double&){}
      virtual bool changeShownDim(){return false;}
      virtual bool isLabelOfFreeAxis(const std::string&) const {return false;}
      SetPeaksWorkspaces presentedWorkspaces() const{SetPeaksWorkspaces empty; return empty;}
      void setForegroundColour(const Qt::GlobalColor){/*Do nothing*/}
      void setBackgroundColour(const Qt::GlobalColor){/*Do nothing*/}
    };

  }
}

#endif /* MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_ */