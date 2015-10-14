#include "MantidQtSliceViewer/SliceViewerStateHandler.h"

namespace MantidQt{
namespace SliceViewer{
    SliceViewer* SV;
    void SliceViewerStateHandler::handleRebinButton(bool state){
        if (state){
            handleStateChange(SV, SLICEVIEWREQUESTS::REQUESTREBINON);
        }
        else{
            handleStateChange(SV, SLICEVIEWREQUESTS::REQUESTREBINOFF);
        }
    }
    void SliceViewerStateHandler::handleStateChange(SliceViewer* SV, SLICEVIEWREQUESTS request){
        auto currentState = SV->getCurrentState();
        currentState->nextState(SV, request);
    }
}
}