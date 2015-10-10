//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/SortEvents.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(SortEvents)

using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;

/** Initialisation method. Declares properties to be used in algorithm.
*
*/
void SortEvents::init() {
  declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace", "",
                                                        Direction::InOut),
                  "EventWorkspace to be sorted.");

  std::vector<std::string> propOptions;
  propOptions.push_back("X Value");
  propOptions.push_back("Pulse Time");
  propOptions.push_back("Pulse Time + TOF");
  declareProperty("SortBy", "X Value",
                  boost::make_shared<StringListValidator>(propOptions),
                  "How to sort the events:\n"
                  "  X Value: the x-position of the event in each pixel "
                  "(typically Time of Flight).\n"
                  "  Pulse Time: the wall-clock time of the pulse that "
                  "produced the event.");
}

/** Executes the rebin algorithm
*
*  @throw runtime_error Thrown if the bin range does not intersect the range of
*the input workspace
*/
void SortEvents::exec() {
  // Get the input workspace
  EventWorkspace_sptr eventW = getProperty("InputWorkspace");
  // And other properties
  std::string sortoption = getPropertyValue("SortBy");

  //------- EventWorkspace ---------------------------
  const size_t histnumber = eventW->getNumberHistograms();

  // Initialize progress reporting.
  Progress prog(this, 0.0, 1.0, histnumber);

  DataObjects::EventSortType sortType = DataObjects::TOF_SORT;
  if (sortoption == "Pulse Time")
    sortType = DataObjects::PULSETIME_SORT;
  else if (sortoption == "Pulse Time + TOF")
    sortType = DataObjects::PULSETIMETOF_SORT;

  // This runs the SortEvents algorithm in parallel
  eventW->sortAll(sortType, &prog);

  return;
}

} // namespace Algorithm
} // namespace Mantid
