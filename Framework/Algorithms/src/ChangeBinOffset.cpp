#include "MantidAlgorithms/ChangeBinOffset.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ChangeBinOffset)

/** Initialisation method. Declares properties to be used in algorithm.
*
*/
void ChangeBinOffset::init() {
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                                         Direction::Input),
                  "Name of the input workspace");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "Name of the output workspace");
  auto isDouble = boost::make_shared<BoundedValidator<double>>();
  declareProperty("Offset", 0.0, isDouble,
                  "The amount to change each time bin by");

  declareSpectrumIndexSetProperties();
}

/** Executes the algorithm
 *
 */
void ChangeBinOffset::exec() {
  // Get input workspace and offset
  const MatrixWorkspace_sptr inputW = getProperty("InputWorkspace");

  offset = getProperty("Offset");

  MatrixWorkspace_sptr outputW = getProperty("OutputWorkspace");
  if (outputW != inputW) {
    outputW = MatrixWorkspace_sptr(inputW->clone().release());
    setProperty("OutputWorkspace", outputW);
  }

  // Check if its an event workspace
  EventWorkspace_const_sptr eventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputW);
  if (eventWS != nullptr) {
    this->execEvent();
    return;
  }

  this->for_each(*outputW, std::make_tuple(Getters::x),
                 [=](std::vector<double> &dataX) {
                   for (auto &x : dataX)
                     x += offset;
                 });

  // Copy units
  if (outputW->getAxis(0)->unit().get())
    outputW->getAxis(0)->unit() = inputW->getAxis(0)->unit();
  try {
    if (inputW->getAxis(1)->unit().get())
      outputW->getAxis(1)->unit() = inputW->getAxis(1)->unit();
  } catch (Exception::IndexError &) {
    // OK, so this isn't a Workspace2D
  }
}

void ChangeBinOffset::execEvent() {
  g_log.information("Processing event workspace");

  MatrixWorkspace_sptr matrixOutputWS = getProperty("OutputWorkspace");
  auto outputWS = boost::dynamic_pointer_cast<EventWorkspace>(matrixOutputWS);

  this->for_each(*outputWS, std::make_tuple(Getters::eventList),
                 [=](EventList &eventList) { eventList.addTof(offset); });

  outputWS->clearMRU();
}

} // namespace Algorithm
} // namespace Mantid
