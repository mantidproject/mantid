// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvToMDSelector.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMDAlgorithms/ConvToMDEventsWSIndexing.h"
#include "MantidMDAlgorithms/ConvToMDHistoWS.h"

namespace Mantid {
namespace MDAlgorithms {
// workspaces which currently can be converted to md workspaces:
enum wsType {
  Matrix2DWS, //< Workspace2D
  EventWS,    //< event workspace
  Undefined   //< unknown initial state
};

ConvToMDSelector::ConvToMDSelector(ConvToMDSelector::ConverterType tp)
    : converterType(tp) {}

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
    throw(std::invalid_argument(
        "ConvToDataObjectsSelector::got a workspace which "
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

  boost::shared_ptr<ConvToMDBase> res;
  // select a converter, which corresponds to the workspace type
  if ((existingWsConvType == Undefined) ||
      (existingWsConvType != inputWSType)) {
    switch (inputWSType) {
    case (EventWS):
      // check if user set a property to use indexing
      if (converterType == ConvToMDSelector::DEFAULT)
        res = boost::make_shared<ConvToMDEventsWS>();
      else
        res = boost::make_shared<ConvToMDEventsWSIndexing>();
      break;
    case (Matrix2DWS):
      res = boost::make_shared<ConvToMDHistoWS>();
      break;
    default:
      throw(std::logic_error("ConvToDataObjectsSelector: requested converter "
                             "for unknown ws type"));
    }
  } else {
    // existing converter is suitable for the workspace
    // in case of Event workspace check if user set a property to use indexing
    if (inputWSType == EventWS) {
      if (converterType == ConvToMDSelector::DEFAULT)
        res = boost::make_shared<ConvToMDEventsWS>();
      else
        res = boost::make_shared<ConvToMDEventsWSIndexing>();
    } else {
      res = boost::make_shared<ConvToMDHistoWS>();
    }
  }

  return res;
}
} // namespace MDAlgorithms
} // namespace Mantid
