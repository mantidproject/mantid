#include "MantidMDEvents/ConvToMDSelector.h"

namespace Mantid {
namespace MDEvents {
// workspaces which currently can be converted to md workspaces:
enum wsType {
  Matrix2DWS, //< Workspace2D
  EventWS,    //< event workspace
  Undefined   //< unknown initial state
};
/** function which selects the convertor depending on workspace type and
(possibly, in a future) some workspace properties
* @param inputWS      -- the sp to workspace which has to be processed
* @param currentSolver -- the sp to the existing solver (may be undef if not
initiated)

*@returns shared pointer to new solver, which corresponds to the workspace
*/
boost::shared_ptr<ConvToMDBase> ConvToMDSelector::convSelector(
    API::MatrixWorkspace_sptr inputWS,
    boost::shared_ptr<ConvToMDBase> &currentSolver) const {
  // identify what kind of workspace we expect to process
  wsType inputWSType = Undefined;
  if (boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(inputWS))
    inputWSType = EventWS;
  if (boost::dynamic_pointer_cast<DataObjects::Workspace2D>(inputWS))
    inputWSType = Matrix2DWS;

  if (inputWSType == Undefined)
    throw(std::invalid_argument("ConvToMDEventsSelector::got a workspace which "
                                "is neither matrix nor event workspace; Can "
                                "not deal with it"));

  // identify what converter (if any) is currently initialized;
  wsType existingWsConvType(Undefined);
  ConvToMDBase *pSolver = currentSolver.get();
  if (pSolver) {
    if (dynamic_cast<ConvToMDEventsWS *>(pSolver))
      existingWsConvType = EventWS;
    if (dynamic_cast<ConvToMDHistoWS *>(pSolver))
      existingWsConvType = Matrix2DWS;
  }

  // select a converter, which corresponds to the workspace type
  if ((existingWsConvType == Undefined) ||
      (existingWsConvType != inputWSType)) {
    switch (inputWSType) {
    case (EventWS):
      return boost::shared_ptr<ConvToMDBase>(new ConvToMDEventsWS());
    case (Matrix2DWS):
      return boost::shared_ptr<ConvToMDBase>(new ConvToMDHistoWS());
    default:
      throw(std::logic_error(
          "ConvToMDEventsSelector: requested converter for unknown ws type"));
    }

  } else { // existing converter is suitable for the workspace
    return currentSolver;
  }
}
} // end MDAlgorithms Namespace
} // end Mantid Namespace