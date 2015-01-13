//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/FilterBadPulses.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/V3D.h"

#include <fstream>
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterBadPulses)

using namespace Kernel;
using namespace API;
using DataObjects::EventList;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_sptr;
using DataObjects::EventWorkspace_const_sptr;
using std::size_t;

namespace { // anonymous namespace for some internal variables
/// Name of log for integrated proton charge
const std::string INT_CHARGE_NAME("gd_prtn_chrg");
/// Name of log for proton charge
const std::string LOG_CHARGE_NAME("proton_charge");
}

//========================================================================
//========================================================================
/// (Empty) Constructor
FilterBadPulses::FilterBadPulses() {}

/// Destructor
FilterBadPulses::~FilterBadPulses() {}
/// Algorithm's name for identification overriding a virtual method
const std::string FilterBadPulses::name() const { return "FilterBadPulses"; }

/// Algorithm's version for identification overriding a virtual method
int FilterBadPulses::version() const { return 1; }

/// Algorithm's category for identification overriding a virtual method
const std::string FilterBadPulses::category() const {
  return "Events\\EventFiltering";
}

//-----------------------------------------------------------------------
/// Initialise the properties
void FilterBadPulses::init() {
  declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "An event workspace");
  declareProperty(new WorkspaceProperty<EventWorkspace>("OutputWorkspace", "",
                                                        Direction::Output),
                  "The name to use for the output workspace");
  auto range = boost::make_shared<BoundedValidator<double>>();
  range->setBounds(0., 100.);
  declareProperty("LowerCutoff", 95., range,
                  "The percentage of the average to use as the lower bound");
}

//-----------------------------------------------------------------------
/** Executes the algorithm */
void FilterBadPulses::exec() {
  // the input workspace into the event workspace we already know it is
  EventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");

  // get the run object
  const API::Run &runlogs = inputWS->run();

  // see if the gd_prtn_charge log has anything useful to say
  if (runlogs.hasProperty(INT_CHARGE_NAME)) {
    double value = runlogs.getPropertyValueAsType<double>(INT_CHARGE_NAME);
    if (value <= 0.) {
      throw std::runtime_error("Found no integrated charge value in " +
                               INT_CHARGE_NAME);
    }
  } else {
    this->g_log.warning() << "Failed to find \"" << INT_CHARGE_NAME
                          << "\" in run object.\n";
  }

  // get the proton charge exists in the run object
  if (!runlogs.hasProperty(LOG_CHARGE_NAME)) {
    throw std::runtime_error("Failed to find \"" + LOG_CHARGE_NAME +
                             "\" in sample logs");
  }
  Kernel::TimeSeriesProperty<double> *pcharge_log =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          runlogs.getLogData(LOG_CHARGE_NAME));
  Kernel::TimeSeriesPropertyStatistics stats = pcharge_log->getStatistics();

  // check that the maximum value is greater than zero
  if (stats.maximum <= 0.) {
    throw std::runtime_error(
        "Maximum value of charge is not greater than zero (" + LOG_CHARGE_NAME +
        ")");
  }

  // set the range
  double min_percent = this->getProperty("LowerCutoff");
  min_percent *= .01; // convert it to a percentage (0<x<1)
  double min_pcharge = stats.mean * min_percent;
  double max_pcharge = stats.maximum * 1.1; // make sure everything high is in
  if (min_pcharge >= max_pcharge) {
    throw std::runtime_error(
        "proton_charge window filters out all of the data");
  }
  this->g_log.information() << "Filtering pcharge outside of " << min_pcharge
                            << " to " << max_pcharge << std::endl;
  size_t inputNumEvents = inputWS->getNumberEvents();

  // Child Algorithme does all of the actual work - do not set the output
  // workspace
  IAlgorithm_sptr filterAlgo = createChildAlgorithm("FilterByLogValue", 0., 1.);
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
  double percent = static_cast<double>(inputNumEvents - outputNumEvents) /
                   static_cast<double>(inputNumEvents);
  percent *= 100.;
  if (percent > 10.) {
    this->g_log.warning() << "Deleted " << (inputNumEvents - outputNumEvents)
                          << " of " << inputNumEvents << " events ("
                          << static_cast<int>(percent) << "%)\n";
  } else {
    this->g_log.notice() << "Deleted " << (inputNumEvents - outputNumEvents)
                         << " of " << inputNumEvents << " events ("
                         << static_cast<float>(percent) << "%)"
                         << " by proton charge from " << min_pcharge << " to "
                         << max_pcharge << " with mean = " << stats.mean
                         << "\n";
  }
}

} // namespace Algorithms
} // namespace Mantid
