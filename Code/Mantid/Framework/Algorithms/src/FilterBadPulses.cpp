/*WIKI* 

The algorithm looks at sample logs ("proton_charge"), finds the mean, and rejects any events that occurred during a pulse that was below a certain percentage of that mean. This effectively removes neutrons from the background that were measured while the accelerator was not actually producing neutrons, reducing background noise.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FilterBadPulses.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/V3D.h"

#include <fstream>
#include "MantidKernel/BoundedValidator.h"

namespace Mantid
{
namespace Algorithms
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterBadPulses)

/// Sets documentation strings for this algorithm
void FilterBadPulses::initDocs()
{
  this->setWikiSummary(" Filters out events associated with pulses that happen when proton charge is lower than a given percentage of the average. ");
  this->setOptionalMessage("Filters out events associated with pulses that happen when proton charge is lower than a given percentage of the average.");
}


using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;
using std::size_t;

const double CONSTANT = (PhysicalConstants::h * 1e10) / (2.0 * PhysicalConstants::NeutronMass * 1e6);

//========================================================================
//========================================================================
/// (Empty) Constructor
FilterBadPulses::FilterBadPulses()
{
}

/// Destructor
FilterBadPulses::~FilterBadPulses()
{
}
/// Algorithm's name for identification overriding a virtual method
const std::string FilterBadPulses::name() const
{
  return "FilterBadPulses";
}

/// Algorithm's version for identification overriding a virtual method
int FilterBadPulses::version() const
{
  return 1;
}

/// Algorithm's category for identification overriding a virtual method
const std::string FilterBadPulses::category() const
{
  return "Events\\EventFiltering";
}

//-----------------------------------------------------------------------
/// Initialise the properties
void FilterBadPulses::init()
{
  declareProperty(
    new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input),
    "An event workspace" );
  declareProperty(
    new WorkspaceProperty<EventWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );
  auto range = boost::make_shared<BoundedValidator<double> >();
  range->setBounds(0., 100.);
  declareProperty("LowerCutoff", 95., range,
                  "The percentage of the average to use as the lower bound");
}


//-----------------------------------------------------------------------
/** Executes the algorithm */
void FilterBadPulses::exec()
{
  // the input workspace into the event workspace we already know it is
  EventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");

  // get the proton charge exists in the run object
  const API::Run& runlogs = inputWS->run();
  if (!runlogs.hasProperty("proton_charge"))
  {
    throw std::runtime_error("Failed to find \"proton_charge\" in sample logs");
  }
  Kernel::TimeSeriesProperty<double> * pcharge_log
      = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( runlogs.getLogData("proton_charge") );
  Kernel::TimeSeriesPropertyStatistics stats = pcharge_log->getStatistics();

  // set the range
  double min_percent = this->getProperty("LowerCutoff");
  min_percent *= .01; // convert it to a percentage (0<x<1)
  double min_pcharge = stats.mean * min_percent;
  double max_pcharge = stats.maximum * 1.1; // make sure everything high is in
  if (min_pcharge >= max_pcharge) {
    throw std::runtime_error("proton_charge window filters out all of the data");
  }
  this->g_log.information() << "Filtering pcharge outside of " << min_pcharge
                            << " to " << max_pcharge << std::endl;
  size_t inputNumEvents = inputWS->getNumberEvents();

  // sub-algorithme does all of the actual work - do not set the output workspace
  IAlgorithm_sptr filterAlgo = createSubAlgorithm("FilterByLogValue", 0., 1.);
  filterAlgo->setProperty("InputWorkspace", inputWS);
  filterAlgo->setProperty("LogName", "proton_charge");
  filterAlgo->setProperty("MinimumValue", min_pcharge);
  filterAlgo->setProperty("MaximumValue", max_pcharge);
  filterAlgo->execute();

  // just grab the child's output workspace
  EventWorkspace_sptr outputWS = filterAlgo->getProperty("OutputWorkspace");
  size_t outputNumEvents = outputWS->getNumberEvents();
  this->setProperty("OutputWorkspace", outputWS);

  // log the number of events deleted
  double percent = static_cast<double>(inputNumEvents - outputNumEvents)
      / static_cast<double>(inputNumEvents);
  percent *= 100.;
  if (percent > 10.)
  {
    this->g_log.warning() << "Deleted " << (inputNumEvents - outputNumEvents)
                          << " of " << inputNumEvents
                          << " events (" << static_cast<int>(percent) << "%)\n";
  }
  else
  {
    this->g_log.information() << "Deleted " << (inputNumEvents - outputNumEvents)
                              << " of " << inputNumEvents
                              << " events (" << static_cast<int>(percent) << "%)\n";
  }
}


} // namespace Algorithms
} // namespace Mantid
