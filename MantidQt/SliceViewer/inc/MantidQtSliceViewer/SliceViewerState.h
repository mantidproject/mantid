#ifndef MANTID_SLICEVIEWER_SLICEVIEWERSTATE_H_
#define MANTID_SLICEVIEWER_SLICEVIEWERSTATE_H_

#include "boost/shared_ptr.hpp"
enum SLICEVIEWREQUESTS {REQUESTREBINON, REQUESTREBINOFF, REQUESTDYNAMICREBINOFF};
namespace MantidQt{
    namespace SliceViewer{
class SliceViewerState;
class SliceViewer;
/// shared pointer to the SliceViewerState base class
typedef boost::shared_ptr<SliceViewerState> SliceViewerState_sptr;
/// shared pointer to the SliceViewerState base class (const version)
typedef boost::shared_ptr<const SliceViewerState> SliceViewerState_const_sptr;

class SliceViewerState{
public:
    virtual ~SliceViewerState(){}
    virtual void nextState(SliceViewer* sliceViewer, SLICEVIEWREQUESTS request) = 0;
    virtual void apply(SliceViewer* sliceViewer) = 0;
};
    }
}

#endif //MANTID_SLICEVIEWER_SLICEVIEWERSTATE_H_