#include "MantidMDEvents/ConvToMDEventsSelector.h"

namespace Mantid
{
namespace MDEvents
{
// workspaces which currently can be converted to md workspaces:
enum wsType
{
    Matrix2DWS, //< Workspace2D
    EventWS,    //< event workspace
    Undefined  //< unknown initial state
};
/** function which selects the convertor depending on workspace type and (possibly, in a future) some workspace properties
  * @param inputWS      -- the sp to workspace which has to be processed
  * @param currentSolver -- the sp to the existing solver (may be null if no initiated)

  *@returns shared pointer to new solver, which corresponds to the workspace
*/
 boost::shared_ptr<ConvToMDEventsBase> ConvToMDEventsSelector::convSelector(API::MatrixWorkspace_sptr inputWS,
                                                                            boost::shared_ptr<ConvToMDEventsBase> currentSolver)const
 {
    // identify what kind of workspace we expect to process
    wsType inputWSType(Undefined);
    if(dynamic_cast<DataObjects::EventWorkspace *>(inputWS.get()))    inputWSType = EventWS;   
    if(dynamic_cast<DataObjects::Workspace2D *>(inputWS.get())   )    inputWSType = Matrix2DWS;
    
    if(inputWSType == Undefined)
    {
        throw(std::invalid_argument("ConvToMDEventsSelector::got a workspace which is neither matrix nor event workspace; Can not deal with it"));
    }

    // identify what converter (if any) is currently initialized;
     wsType  wsConvType(Undefined);
     ConvToMDEventsBase *pSolver = currentSolver.get();
     if(pSolver)
     {
         if(dynamic_cast<ConvToMDEventsEvents *>(pSolver))    wsConvType = EventWS;
         if(dynamic_cast<ConvToMDEventsHisto  *>(pSolver))    wsConvType = Matrix2DWS;          
     }

     // select a converter, which corresponds to the workspace type
     if((wsConvType==Undefined)||(wsConvType!=inputWSType))
     {
         switch(inputWSType)
         {
            case(EventWS):       return boost::shared_ptr<ConvToMDEventsBase>(new ConvToMDEventsEvents());
            case(Matrix2DWS):    return boost::shared_ptr<ConvToMDEventsBase>(new ConvToMDEventsHisto());
            default:             throw(std::logic_error("ConvToMDEventsSelector: requested converter for unknown ws type"));
         }

     }else{ // existing converter is suitable for the workspace
         return currentSolver;
     }
 }
} // end MDAlgorithms Namespace
} // end Mantid Namespace