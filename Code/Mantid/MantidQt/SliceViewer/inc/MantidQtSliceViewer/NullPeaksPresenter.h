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
      /*virtual QColor getForegroundColour() const{ throw std::logic_error("NullPeaksPresenter::getForegroundColour not possible.");};
      virtual QColor getBackgroundColour() const{ throw std::logic_error("NullPeaksPresenter::getBackgroundColour not possible");};*/
    };

  }
}

#endif /* MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_ */