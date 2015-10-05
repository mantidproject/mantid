#include "MantidAlgorithms/ExportTimeSeriesLog.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IEventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <algorithm>
#include <fstream>
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(ExportTimeSeriesLog)
//----------------------------------------------------------------------------------------------
/** Constructor
 */
ExportTimeSeriesLog::ExportTimeSeriesLog() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ExportTimeSeriesLog::~ExportTimeSeriesLog() {}

//----------------------------------------------------------------------------------------------
/** Definition of all input arguments
 */
void ExportTimeSeriesLog::init() {
  declareProperty(
      new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous",
                                                  Direction::InOut),
      "Name of input Matrix workspace containing the log to export. ");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "Dummy",
                                             Direction::Output),
      "Name of the workspace containing the log events in Export. ");

  declareProperty("LogName", "", "Log's name to filter events.");

  declareProperty(
      "NumberEntriesExport", EMPTY_INT(),
      "Number of entries of the log to be exported.  Default is all entries.");

  declareProperty("IsEventWorkspace", true, "If set to true, output workspace "
                                            "is EventWorkspace.  Otherwise, it "
                                            "is Workspace2D.");

  return;
}

//----------------------------------------------------------------------------------------------
/** Main execution
 */
void ExportTimeSeriesLog::exec() {
  // 1. Get property
  m_dataWS = this->getProperty("InputWorkspace");

  string logname = getProperty("LogName");
  int numberoutputentries = getProperty("NumberEntriesExport");
  bool outputeventworkspace = getProperty("IsEventWorkspace");

  // 2. Call the main
  exportLog(logname, numberoutputentries, outputeventworkspace);

  // 3. Output
  setProperty("OutputWorkspace", m_outWS);

  return;
}

//----------------------------------------------------------------------------------------------
/** Export part of designated log to an file in column format and a output file
  * @param logname :: name of log to export
  * @param numentries :: number of log entries to export
  * @param outputeventws :: boolean.  output workspace is event workspace if
 * true.
 */
void ExportTimeSeriesLog::exportLog(string logname, int numentries,
                                    bool outputeventws) {

  // 1.  Get log, time, and etc.
  std::vector<Kernel::DateAndTime> times;
  std::vector<double> values;

  if (logname.size() > 0) {
    // Log
    Kernel::TimeSeriesProperty<double> *tlog =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
            m_dataWS->run().getProperty(logname));
    if (!tlog) {
      std::stringstream errmsg;
      errmsg << "TimeSeriesProperty Log " << logname
             << " does not exist in workspace " << m_dataWS->getName();
      g_log.error(errmsg.str());
      throw std::invalid_argument(errmsg.str());
    }
    times = tlog->timesAsVector();
    values = tlog->valuesAsVector();
  } else {
    throw std::runtime_error("Log name cannot be left empty.");
  }

  // 2. Determine number of export log
  if (numentries == EMPTY_INT()) {
    numentries = static_cast<int>(times.size());
  } else if (numentries <= 0) {
    stringstream errmsg;
    errmsg << "For Export Log, NumberEntriesExport must be greater than 0.  "
              "Input = " << numentries;
    g_log.error(errmsg.str());
    throw std::runtime_error(errmsg.str());
  } else if (static_cast<size_t>(numentries) > times.size()) {
    numentries = static_cast<int>(times.size());
  }

  // 3. Create otuput workspace
  if (outputeventws) {
    setupEventWorkspace(numentries, times, values);
  } else {
    setupWorkspace2D(numentries, times, values);
  }

  return;
}

/** Set up the output workspace in a Workspace2D
  * @param numentries :: number of log entries to output
  * @param times :: vector of Kernel::DateAndTime
  * @param values :: vector of log value in double
  */
void ExportTimeSeriesLog::setupWorkspace2D(int numentries,
                                           vector<DateAndTime> &times,
                                           vector<double> values) {
  Kernel::DateAndTime runstart(
      m_dataWS->run().getProperty("run_start")->value());

  size_t size = static_cast<size_t>(numentries);
  m_outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, size, size));
  if (!m_outWS)
    throw runtime_error(
        "Unable to create a Workspace2D casted to MatrixWorkspace.");

  MantidVec &vecX = m_outWS->dataX(0);
  MantidVec &vecY = m_outWS->dataY(0);
  MantidVec &vecE = m_outWS->dataE(0);

  for (size_t i = 0; i < size; ++i) {
    int64_t dtns = times[i].totalNanoseconds() - runstart.totalNanoseconds();
    vecX[i] = static_cast<double>(dtns) * 1.0E-9;
    vecY[i] = values[i];
    vecE[i] = 0.0;
  }

  Axis *xaxis = m_outWS->getAxis(0);
  xaxis->setUnit("Time");

  return;
}

//----------------------------------------------------------------------------------------------
/** Set up an Event workspace
  * @param numentries :: number of log entries to output
  * @param times :: vector of Kernel::DateAndTime
  * @param values :: vector of log value in double
  */
void ExportTimeSeriesLog::setupEventWorkspace(int numentries,
                                              vector<DateAndTime> &times,
                                              vector<double> values) {
  Kernel::DateAndTime runstart(
      m_dataWS->run().getProperty("run_start")->value());

  // Get some stuff from the input workspace
  const size_t numberOfSpectra = 1;
  const int YLength = static_cast<int>(m_dataWS->blocksize());

  // Make a brand new EventWorkspace
  EventWorkspace_sptr outEventWS = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", numberOfSpectra, YLength + 1, YLength));
  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(m_dataWS, outEventWS,
                                                         false);

  m_outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outEventWS);
  if (!m_outWS)
    throw runtime_error(
        "Output workspace cannot be casted to a MatrixWorkspace.");

  g_log.debug("[DBx336] An output workspace is generated.!");

  // Create the output event list (empty)
  EventList &outEL = outEventWS->getOrAddEventList(0);
  outEL.switchTo(WEIGHTED_NOTIME);

  // Allocate all the required memory
  outEL.reserve(numentries);
  outEL.clearDetectorIDs();

  for (size_t i = 0; i < static_cast<size_t>(numentries); i++) {
    Kernel::DateAndTime tnow = times[i];
    int64_t dt = tnow.totalNanoseconds() - runstart.totalNanoseconds();

    // convert to microseconds
    double dtmsec = static_cast<double>(dt) / 1000.0;
    outEL.addEventQuickly(WeightedEventNoTime(dtmsec, values[i], values[i]));
  }
  // Ensure thread-safety
  outEventWS->sortAll(TOF_SORT, NULL);

  // Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(2);
  std::vector<WeightedEventNoTime> &events = outEL.getWeightedEventsNoTime();
  xRef[0] = events.begin()->tof();
  xRef[1] = events.rbegin()->tof();

  // Set the binning axis using this.
  outEventWS->setX(0, axis);

  return;
}

} // namespace Mantid
} // namespace Algorithms
