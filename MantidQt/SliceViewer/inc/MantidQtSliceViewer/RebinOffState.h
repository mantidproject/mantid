#ifndef MANTID_SLICEVIEWER_REBINOFFSTATE_H_
#define MANTID_SLICEVIEWER_REBINOFFSTATE_H_

#include "MantidQtSliceViewer/SliceViewerState.h"
namespace MantidQt{
namespace SliceViewer{
class SliceViewer;
class RebinOffState : public SliceViewerState{
public:
    RebinOffState(){}
    virtual ~RebinOffState(){}
        void nextState(SliceViewer* sliceViewer, SLICEVIEWREQUESTS request);
        void apply(SliceViewer* sliceViewer);
};
}
}

#endif //MANTID_SLICEVIEWER_REBINOFFSTATE_H_