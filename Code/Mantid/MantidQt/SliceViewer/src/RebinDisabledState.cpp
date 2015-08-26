#include"MantidQtSliceViewer/RebinDisabledState.h"
#include "MantidQtSliceViewer/RebinOffState.h"
#include "MantidQtSliceViewer/SliceViewer.h"
namespace MantidQt{
namespace SliceViewer{
class RebinOffState;
    void RebinDisabledState::nextState(SliceViewer* sliceViewer, SLICEVIEWREQUESTS request){
        if (request == SLICEVIEWREQUESTS::REQUESTREBINOFF){
            SliceViewerState_sptr state = boost::make_shared<RebinOffState>();
            if(sliceViewer->m_WS_is_EventWorkspace()){
                sliceViewer->setCurrentState(state);
            }
        }
}
void RebinDisabledState::apply(SliceViewer* sliceViewer){
    sliceViewer->setRebinMode(false, false); //Rebin mode button is now enabled
    //update gui

}
}
}
