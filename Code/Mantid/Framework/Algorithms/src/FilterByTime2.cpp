#include "MantidAlgorithms/FilterByTime2.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidDataObjects/SplittersWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FilterByTime2::FilterByTime2() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FilterByTime2::~FilterByTime2() {}

//-----------------------------------------------------------------------
void FilterByTime2::init() {
  std::string commonHelp("\nYou can only specify the relative or absolute "
                         "start/stop times, not both.");

  declareProperty(new WorkspaceProperty<DataObjects::EventWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(new WorkspaceProperty<DataObjects::EventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");

  auto min = boost::make_shared<BoundedValidator<double>>();
  min->setLower(0.0);
  declareProperty("StartTime", 0.0, min,
                  "The start time, in seconds, since the start of the run. "
                  "Events before this time are filtered out. \nThe time of the "
                  "first pulse (i.e. the first entry in the ProtonCharge "
                  "sample log) is used as the zero. " +
                      commonHelp);

  declareProperty("StopTime", 0.0,
                  "The stop time, in seconds, since the start of the run. "
                  "Events at or after this time are filtered out. \nThe time "
                  "of the first pulse (i.e. the first entry in the "
                  "ProtonCharge sample log) is used as the zero. " +
                      commonHelp);

  std::string absoluteHelp(
      "Specify date and UTC time in ISO8601 format, e.g. 2010-09-14T04:20:12." +
      commonHelp);
  declareProperty(
      "AbsoluteStartTime", "",
      "Absolute start time; events before this time are filtered out. " +
          absoluteHelp);

  declareProperty(
      "AbsoluteStopTime", "",
      "Absolute stop time; events at of after this time are filtered out. " +
          absoluteHelp);
}

//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 */
void FilterByTime2::exec() {
  DataObjects::EventWorkspace_const_sptr inWS =
      this->getProperty("InputWorkspace");
  if (!inWS) {
    g_log.error() << "Input is not EventWorkspace" << std::endl;
    throw std::invalid_argument("Input is not EventWorksapce");
  } else {
    g_log.debug() << "DB5244 InputWorkspace Name = " << inWS->getName()
                  << std::endl;
  }

  double starttime = this->getProperty("StartTime");
  double stoptime = this->getProperty("StopTime");
  std::string absstarttime = this->getProperty("AbsoluteStartTime");
  std::string absstoptime = this->getProperty("AbsoluteStopTime");

  std::string start, stop;
  if ((absstarttime != "") && (absstoptime != "") && (starttime <= 0.0) &&
      (stoptime <= 0.0)) {
    // Use the absolute string
    start = absstarttime;
    stop = absstoptime;
  } else if ((absstarttime != "" || absstoptime != "") &&
             (starttime > 0.0 || stoptime > 0.0)) {
    throw std::invalid_argument(
        "It is not allowed to provide both absolute time and relative time.");
  } else {
    // Use second
    std::stringstream ss;
    ss << starttime;
    start = ss.str();
    std::stringstream ss2;
    ss2 << stoptime;
    stop = ss2.str();
  }

  // 1. Generate Filters
  g_log.debug() << "\nDB441: About to generate Filter.  StartTime = "
                << starttime << "  StopTime = " << stoptime << std::endl;

  API::Algorithm_sptr genfilter =
      createChildAlgorithm("GenerateEventsFilter", 0.0, 20.0, true, 1);
  genfilter->initialize();
  genfilter->setPropertyValue("InputWorkspace", inWS->getName());
  genfilter->setPropertyValue("OutputWorkspace", "FilterWS");
  genfilter->setProperty("StartTime", start);
  genfilter->setProperty("StopTime", stop);
  genfilter->setProperty("UnitOfTime", "Seconds");
  genfilter->setProperty("FastLog", false);

  bool sucgen = genfilter->execute();
  if (!sucgen) {
    g_log.error() << "Unable to generate event filters" << std::endl;
    throw std::runtime_error("Unable to generate event filters");
  } else {
    g_log.debug() << "Filters are generated. " << std::endl;
  }

  API::Workspace_sptr filterWS = genfilter->getProperty("OutputWorkspace");
  if (!filterWS) {
    g_log.error() << "Unable to retrieve generated SplittersWorkspace object "
                     "from AnalysisDataService." << std::endl;
    throw std::runtime_error("Unable to retrieve Splittersworkspace. ");
  }

  // 2. Filter events
  g_log.debug() << "\nAbout to filter events. "
                << "\n";

  API::Algorithm_sptr filter =
      createChildAlgorithm("FilterEvents", 20.0, 100.0, true, 1);
  filter->initialize();
  filter->setPropertyValue("InputWorkspace", inWS->getName());
  filter->setPropertyValue("OutputWorkspaceBaseName", "ResultWS");
  filter->setProperty("SplitterWorkspace", filterWS);
  filter->setProperty("FilterByPulseTime", true);

  bool sucfilt = filter->execute();
  if (!sucfilt) {
    g_log.error() << "Unable to filter events" << std::endl;
    throw std::runtime_error("Unable to filter events");
  } else {
    g_log.debug() << "Filter events is successful. " << std::endl;
  }

  DataObjects::EventWorkspace_sptr optws =
      filter->getProperty("OutputWorkspace_0");

  this->setProperty("OutputWorkspace", optws);
}

} // namespace Mantid
} // namespace Algorithms
