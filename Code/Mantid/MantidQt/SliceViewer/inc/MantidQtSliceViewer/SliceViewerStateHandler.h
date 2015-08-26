#include "MantidQtSliceViewer/SliceViewerState.h"

namespace MantidQt{
namespace SliceViewer{
class SliceViewerStateHandler{

public slots:
//Methods for dealing with the signals recieved from SliceViewer
  void handleRebinButton(bool state);
  void handleDynamicRebinButton(bool state);
public :
    void handleStateChange(SliceViewer* sliceViewer,SLICEVIEWREQUESTS request);
    //auto state = sliceViewer->getCurrentState();
    //state->nextState(sliceViewer, request)
};
}
}