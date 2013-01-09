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
      void setForegroundColour(const QColor){/*Do nothing*/}
      void setBackgroundColour(const QColor){/*Do nothing*/}
      std::string getTransformName() const {return "";}
      void showBackgroundRadius(const bool){/*Do nothing*/}
      void setShown(const bool){/*Do nothing*/}
      virtual RectangleType getBoundingBox(const int) const{return boost::make_tuple(Mantid::Kernel::V2D(), Mantid::Kernel::V2D());}
    };

  }
}

#endif /* MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_ */