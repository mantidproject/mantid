// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FilterBadPulses.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"

namespace Mantid::Algorithms {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FilterBadPulses)

using namespace Kernel;
using namespace API;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;
using std::size_t;

namespace { // anonymous namespace for some internal variables
/// Name of log for integrated proton charge
const std::string INT_CHARGE_NAME("gd_prtn_chrg");
/// Name of log for proton charge
const std::string LOG_CHARGE_NAME("proton_charge");
} // namespace

/// Algorithm's name for identification overriding a virtual method
const std::string FilterBadPulses::name() const { return "FilterBadPulses"; }

/// Algorithm's version for identification overriding a virtual method
int FilterBadPulses::version() const { return 1; }

/// Algorithm's category for identification overriding a virtual method
const std::string FilterBadPulses::category() const { return "Events\\EventFiltering"; }

/// Initialise the properties
void FilterBadPulses::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An event workspace");
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
  auto range = std::make_shared<BoundedValidator<double>>();
  range->setBounds(0., 100.);
  declareProperty("LowerCutoff", 95., range, "The percentage of the average to use as the lower bound");
}

/** Executes the algorithm */
void FilterBadPulses::exec() {
  // the input workspace into the event workspace we already know it is
  EventWorkspace_sptr inputWS = this->getProperty("InputWorkspace");

  // get the run object
  const API::Run &runlogs = inputWS->run();

  // see if the gd_prtn_charge log has anything useful to say
  if (runlogs.hasProperty(INT_CHARGE_NAME)) {
    auto value = runlogs.getPropertyValueAsType<double>(INT_CHARGE_NAME);
    if (value <= 0.) {
      throw std::runtime_error("Found no integrated charge value in " + INT_CHARGE_NAME);
    }
  } else {
    this->g_log.warning() << "Failed to find \"" << INT_CHARGE_NAME << "\" in run object.\n";
  }

  const auto [min_pcharge, max_pcharge, mean] = runlogs.getBadPulseRange(LOG_CHARGE_NAME, getProperty("LowerCutoff"));

  this->g_log.information() << "Filtering pcharge outside of " << min_pcharge << " to " << max_pcharge << '\n';
  size_t inputNumEvents = inputWS->getNumberEvents();

  // Child Algorithme does all of the actual work - do not set the output
  // workspace
  auto filterAlgo = createChildAlgorithm("FilterByLogValue", 0., 1.);
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
  double percent = static_cast<double>(inputNumEvents - outputNumEvents) / static_cast<double>(inputNumEvents);
  percent *= 100.;
  if (percent > 10.) {
    this->g_log.warning() << "Deleted " << (inputNumEvents - outputNumEvents) << " of " << inputNumEvents << " events ("
                          << static_cast<int>(percent) << "%)\n";
  } else {
    this->g_log.notice() << "Deleted " << (inputNumEvents - outputNumEvents) << " of " << inputNumEvents << " events ("
                         << static_cast<float>(percent) << "%)"
                         << " by proton charge from " << min_pcharge << " to " << max_pcharge << " with mean = " << mean
                         << "\n";
  }
}

} // namespace Mantid::Algorithms
