// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SumEventsByLogValue.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/TimeROI.h"
#include "MantidKernel/VectorHelper.h"

#include <numeric>

namespace Mantid::Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(SumEventsByLogValue)

using namespace Kernel;
using namespace API;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;

void SumEventsByLogValue::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::EventWorkspace>>("InputWorkspace", "", Direction::Input),
      "The input EventWorkspace. Must contain 'raw' (unweighted) events");
  declareProperty(std::make_unique<WorkspaceProperty<DataObjects::EventWorkspace>>(
                      "MonitorWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "A workspace containing the monitor counts relating to the input "
                  "workspace");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm. The output workspace will be a "
                  "[[TableWorkspace]] in the case that a log holding integer "
                  "values is given, and a single-spectrum [[Workspace2D]] "
                  "otherwise.");

  declareProperty("LogName", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the number series log against which the data "
                  "should be summed");
  declareProperty(
      std::make_unique<ArrayProperty<double>>("OutputBinning", "", std::make_shared<RebinParamsValidator>(true)),
      "Binning parameters for the output workspace (see [[Rebin]] for syntax) "
      "(Optional for logs holding integer values, mandatory otherwise)");
}

std::map<std::string, std::string> SumEventsByLogValue::validateInputs() {
  std::map<std::string, std::string> errors;

  // check for null pointers - this is to protect against workspace groups
  m_inputWorkspace = getProperty("InputWorkspace");
  if (!m_inputWorkspace) {
    return errors;
  }

  // This only works for unweighted events
  // TODO: Either turn this check into a proper validator or amend the algorithm
  // to work for weighted events
  if (m_inputWorkspace->getEventType() != API::TOF) {
    errors["InputWorkspace"] = "This algorithm only works for unweighted ('raw') events";
  }

  // Check that the log exists for the given input workspace
  m_logName = getPropertyValue("LogName");
  try {
    auto *log = dynamic_cast<ITimeSeriesProperty *>(m_inputWorkspace->run().getLogData(m_logName));
    if (log == nullptr) {
      errors["LogName"] = "'" + m_logName + "' is not a time-series log.";
      return errors;
    }
    if (log->realSize() == 0) {
      errors["LogName"] = "'" + m_logName + "' is empty.";
    }
  } catch (Exception::NotFoundError &) {
    errors["LogName"] =
        "The log '" + m_logName + "' does not exist in the workspace '" + m_inputWorkspace->getName() + "'.";
    return errors;
  }

  return errors;
}

void SumEventsByLogValue::exec() {
  // Get hold of the requested log. Will throw if it doesn't exist (which is
  // what we want).
  const Property *const log = m_inputWorkspace->run().getLogData(m_logName);

  // Now we need to know what type of property it is
  const auto *intLog = dynamic_cast<const TimeSeriesProperty<int> *>(log);
  const auto *dblLog = dynamic_cast<const TimeSeriesProperty<double> *>(log);

  m_binningParams = getProperty("OutputBinning");
  // Binning parameters must be provided for floating point logs
  if (m_binningParams.empty()) {
    if (intLog != nullptr) {
      createTableOutput(intLog);
    } else {
      throw std::invalid_argument("OutputBinning must be provided for floating-point number logs");
    }
  } else // Binning parameters have been given
  {
    if (intLog != nullptr) {
      createBinnedOutput(intLog);
    } else if (dblLog != nullptr) {
      createBinnedOutput(dblLog);
    }
    // else if ( dynamic_cast<const TimeSeriesProperty<std::string>*>(log) !=
    // NULL )
    //{
    //  TODO: Implement (if anyone ever asks for it).
    //}
    else {
      throw std::runtime_error("This algorithm only supports number-series logs");
    }
  }
}

/** Produces the table workspace output for an integer TimeSeriesProperty.
 *  @param log The log to tabulate against
 */
void SumEventsByLogValue::createTableOutput(const Kernel::TimeSeriesProperty<int> *log) {
  // This is the version for integer logs when no binning parameters have been
  // given and has a data point per log value
  const int minVal = log->minValue();
  const int maxVal = log->maxValue();
  const auto xLength = std::size_t(maxVal - minVal + 1);

  if (xLength > 10000) {
    g_log.warning() << "Did you really want to create a " << xLength << " row table? This will take some time!\n";
  }

  // Accumulate things in a local vector before transferring to the table
  std::vector<int> Y(xLength);
  const auto numSpec = static_cast<int>(m_inputWorkspace->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, std::size_t(numSpec) + xLength);
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWorkspace))
  for (int spec = 0; spec < numSpec; ++spec) {
    PARALLEL_START_INTERRUPT_REGION
    const IEventList &eventList = m_inputWorkspace->getSpectrum(std::size_t(spec));
    filterEventList(eventList, minVal, maxVal, log, Y);
    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Create a table workspace to hold the sum.
  ITableWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().createTable();
  auto logValues = outputWorkspace->addColumn("int", m_logName);
  auto counts = outputWorkspace->addColumn("int", "Counts");
  auto errors = outputWorkspace->addColumn("double", "Error");
  outputWorkspace->setRowCount(xLength); // One row per log value across the full range
  // Set plot type
  logValues->setPlotType(1); // X
  counts->setPlotType(2);    // Y
  errors->setPlotType(5);    // E

  // Transfer the results to the table
  for (std::size_t i = 0; i < xLength; ++i) {
    logValues->cell<int>(i) = minVal + int(i);
    counts->cell<int>(i) = Y[i];
    errors->cell<double>(i) = std::sqrt(Y[i]);
  }

  // Columns for normalisation: monitors (if available), time & proton charge
  addMonitorCounts(outputWorkspace, log, minVal, maxVal);
  // Add a column to hold the time duration (in seconds) for which the log had a
  // certain value
  auto timeCol = outputWorkspace->addColumn("double", "time");
  // Add a column to hold the proton charge for which the log had a certain
  // value
  auto protonChgCol = outputWorkspace->addColumn("double", "proton_charge");
  // Get hold of the proton charge log for later
  const TimeSeriesProperty<double> *protonChargeLog = nullptr;
  try {
    protonChargeLog = m_inputWorkspace->run().getTimeSeriesProperty<double>("proton_charge");
    // Set back to NULL if the log is empty or bad things will happen later
    if (protonChargeLog->realSize() == 0)
      protonChargeLog = nullptr;
  } catch (std::exception &) {
    // Log and carry on if not found. Column will be left empty.
    g_log.warning("proton_charge log not found in workspace.");
  }

  // Get a list of the other time-series logs in the input workspace
  auto otherLogs = getNumberSeriesLogs();
  // Add a column for each of these 'other' logs
  for (const auto &otherLog : otherLogs) {
    auto newColumn = outputWorkspace->addColumn("double", otherLog.first);
    // Set these columns to be containing X values
    newColumn->setPlotType(1);
  }

  // Now to get the average value of other time-varying logs
  // Loop through the values of the 'main' log
  for (int value = minVal; value <= maxVal; ++value) {
    const auto row = std::size_t(value - minVal);
    // Create a filter giving the times when this log has the current value

    TimeROI timeRoi;
    const TimeROI *temp = &m_inputWorkspace->run().getTimeROI();
    // This section ensures that the filter goes to the end of the run
    if (value == log->lastValue() && protonChargeLog) {
      const TimeInterval timeAfterLastLogValue(log->lastTime(), m_inputWorkspace->getLastPulseTime());
      timeRoi = log->makeFilterByValue(value, value, true, timeAfterLastLogValue, 0.0, false, temp);
    } else {
      timeRoi = log->makeFilterByValue(value, value, false, TimeInterval(0, 1), 0., false, temp);
    }

    // Calculate the time covered by this log value and add it to the table
    Run run(m_inputWorkspace->run());
    run.setTimeROI(timeRoi);
    run.removeDataOutsideTimeROI();
    timeCol->cell<double>(row) = run.getTimeROI().durationInSeconds();

    interruption_point();
    // Sum up the proton charge for this log value
    if (protonChargeLog) {
      /// Conversion factor between microAmpere-hours and picoCoulombs
      /// this is required to correctly represent the proton charge
      const double currentConversion = 1.e-6 / 3600.;
      protonChgCol->cell<double>(row) = run.getProtonCharge() / currentConversion;
    }
    interruption_point();

    // filter the logs
    for (const auto &otherLog : otherLogs) {
      // Calculate the average value of each 'other' log for the current value
      // of the main log
      // Have to (maybe inefficiently) fetch back column by name - move outside
      // loop if too slow
      outputWorkspace->getColumn(otherLog.first)->cell<double>(row) = run.getTimeAveragedValue(otherLog.first);
    }

    prog.report();
  }

  setProperty("OutputWorkspace", outputWorkspace);
}

/** Goes through an event list assigning events to the output vector according
 * to the log value
 *  at the time the event was measured. Used for integer logs, where each
 * possible value between
 *  the min & max log value has an entry in the output vector.
 *  @param eventList The event list to parse
 *  @param minVal    The minimum value of the log
 *  @param maxVal    The maximum value of the log
 *  @param log       The TimeSeriesProperty log
 *  @param Y         The output vector to be filled
 */
void SumEventsByLogValue::filterEventList(const API::IEventList &eventList, const int minVal, const int maxVal,
                                          const Kernel::TimeSeriesProperty<int> *log, std::vector<int> &Y) {
  if (log->realSize() == 0)
    return;

  const auto pulseTimes = eventList.getPulseTimes();
  for (const auto &pulseTime : pulseTimes) {
    // Find the value of the log at the time of this event
    // This algorithm is really concerned with 'slow' logs so we don't care
    // about
    // the time of the event within the pulse.
    // NB: If the pulse time is before the first log entry, we get the first
    // value.
    const int logValue = log->getSingleValue(pulseTime);

    if (logValue >= minVal && logValue <= maxVal) {
      // In this scenario it's easy to know what bin to increment
      PARALLEL_ATOMIC
      ++Y[logValue - minVal];
    }
  }
}

/** Looks for monitor event data and, if found, adds columns to the output table
 * corresponding
 *  to the monitor counts for each (integer) log value.
 *  @param outputWorkspace The output table
 *  @param log             The log being summed against
 *  @param minVal          The minimum value of the log
 *  @param maxVal          The maximum value of the log
 */
void SumEventsByLogValue::addMonitorCounts(const ITableWorkspace_sptr &outputWorkspace,
                                           const TimeSeriesProperty<int> *log, const int minVal, const int maxVal) {
  DataObjects::EventWorkspace_const_sptr monitorWorkspace = getProperty("MonitorWorkspace");
  // If no monitor workspace was given, there's nothing to do
  if (!monitorWorkspace)
    return;

  const auto &spectrumInfo = monitorWorkspace->spectrumInfo();

  const auto xLength = std::size_t(maxVal - minVal + 1);
  // Loop over the spectra - there will be one per monitor
  for (std::size_t spec = 0; spec < monitorWorkspace->getNumberHistograms(); ++spec) {
    try {
      // Create a column for this monitor
      const std::string monitorName = spectrumInfo.detector(spec).getName();
      auto monitorCounts = outputWorkspace->addColumn("int", monitorName);
      const IEventList &eventList = monitorWorkspace->getSpectrum(spec);
      // Accumulate things in a local vector before transferring to the table
      // workspace
      std::vector<int> Y(xLength);
      filterEventList(eventList, minVal, maxVal, log, Y);
      // Transfer the results to the table
      for (std::size_t i = 0; i < xLength; ++i) {
        monitorCounts->cell<int>(i) = Y[i];
      }
    } catch (Exception::NotFoundError &) {
      // ADARA-generated nexus files have sometimes been seen to contain
      // 'phantom' monitors that aren't in the IDF.
      // This handles that by ignoring those spectra.
    }
  }
}

/** Searches the input workspace for int or double TimeSeriesProperty's other
 * than
 *  the one being summed against.
 *  @return A list holding the names of the found logs and pointers to the
 * corresponding properties
 */
std::vector<std::pair<std::string, const Kernel::ITimeSeriesProperty *>> SumEventsByLogValue::getNumberSeriesLogs() {
  std::vector<std::pair<std::string, const Kernel::ITimeSeriesProperty *>> numberSeriesProps;
  const auto &logs = m_inputWorkspace->run().getLogData();
  for (const auto &log : logs) {
    const std::string logName = log->name();
    // Don't add the log that's the one being summed against
    if (logName == m_logName)
      continue;
    // Exclude the proton charge log as we have a separate column for the sum of
    // that
    if (logName == "proton_charge")
      continue;
    // Try to cast to an ITimeSeriesProperty
    auto tsp = dynamic_cast<const ITimeSeriesProperty *>(log);
    // Move on to the next one if this is not a TSP
    if (tsp == nullptr)
      continue;
    // Don't keep ones with only one entry
    // if ( tsp->realSize() < 2 ) continue;
    // Now make sure it's either an int or double tsp, and if so add log to the
    // list
    if (dynamic_cast<TimeSeriesProperty<double> *>(log) || dynamic_cast<TimeSeriesProperty<int> *>(log)) {
      numberSeriesProps.emplace_back(logName, tsp);
    }
  }

  return numberSeriesProps;
}

/** Create a single-spectrum Workspace2D containing the integrated counts versus
 *  log value, binned according to the parameters input to the algorithm.
 *  @param log The log against which to count the events.
 */
template <typename T> void SumEventsByLogValue::createBinnedOutput(const Kernel::TimeSeriesProperty<T> *log) {
  // If only the number of bins was given, add the min & max values of the log
  if (m_binningParams.size() == 1) {
    m_binningParams.insert(m_binningParams.begin(), log->minValue());
    m_binningParams.emplace_back(log->maxValue() * 1.000001); // Make it a tiny bit larger to cover full range
  }

  // XValues will be resized in createAxisFromRebinParams()
  std::vector<double> XValues;
  const int XLength = VectorHelper::createAxisFromRebinParams(m_binningParams, XValues);
  assert((int)XValues.size() == XLength);

  // Create the output workspace - the factory will give back a Workspace2D
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D", 1, XLength, XLength - 1);
  // Copy the bin boundaries into the output workspace
  outputWorkspace->mutableX(0) = XValues;
  outputWorkspace->getAxis(0)->title() = m_logName;
  outputWorkspace->setYUnit("Counts");

  auto &Y = outputWorkspace->mutableY(0);
  const auto numSpec = static_cast<int>(m_inputWorkspace->getNumberHistograms());
  Progress prog(this, 0.0, 1.0, numSpec);
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWorkspace))
  for (int spec = 0; spec < numSpec; ++spec) {
    PARALLEL_START_INTERRUPT_REGION
    const IEventList &eventList = m_inputWorkspace->getSpectrum(spec);
    const auto pulseTimes = eventList.getPulseTimes();
    for (const auto &pulseTime : pulseTimes) {
      // Find the value of the log at the time of this event
      const double logValue = log->getSingleValue(pulseTime);
      if (logValue >= XValues.front() && logValue < XValues.back()) {
        PARALLEL_ATOMIC
        ++Y[VectorHelper::getBinIndex(XValues, logValue)];
      }
    }

    prog.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // The errors are the sqrt of the counts so long as we don't deal with
  // weighted events.
  std::transform(Y.cbegin(), Y.cend(), outputWorkspace->mutableE(0).begin(), (double (*)(double))std::sqrt);

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Mantid::Algorithms
