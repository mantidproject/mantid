#ifndef MANTID_SLICEVIEWER_REBINDISABLEDSTATE_H_
#define MANTID_SLICEVIEWER_REBINDISABLEDSTATE_H_

#include "MantidQtSliceViewer/SliceViewerState.h"
namespace MantidQt{
namespace SliceViewer{
class SliceViewer;
class RebinDisabledState : public SliceViewerState{
public:
    void nextState(SliceViewer* sliceViewer, SLICEVIEWREQUESTS request);
    void apply(SliceViewer* sliceViewer);
};
}
}

#endif // !MANTID_SLICEVIEWER_REBINOFFSTATE_H_
