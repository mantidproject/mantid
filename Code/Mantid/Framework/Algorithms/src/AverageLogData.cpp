#include "MantidAlgorithms/AverageLogData.h"
#include "MantidKernel/TimeSeriesProperty.h"
using namespace Mantid::Kernel;
using namespace Mantid::API;
namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AverageLogData)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
AverageLogData::AverageLogData() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
AverageLogData::~AverageLogData() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string AverageLogData::name() const { return "AverageLogData"; }

/// Algorithm's version for identification. @see Algorithm::version
int AverageLogData::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AverageLogData::category() const {
  return "DataHandling\\Logs";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AverageLogData::init() {
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace that contains a Sample log property, and "
                  "a proton charge property.");
  declareProperty("LogName", "", "Name of the log to be averaged");
  declareProperty("FixZero", true, "If true, the proton charge and the log "
                                   "value time series are assumed to start at "
                                   "the same moment.");
  declareProperty("Average", EMPTY_DBL(), "", Direction::Output);
  declareProperty("Error", EMPTY_DBL(), "", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AverageLogData::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  std::string logname = this->getProperty("LogName");
  if (logname.empty()) {
    throw std::runtime_error("Failed to supply a LogName");
  }
  if (!inputWS->run().hasProperty(logname)) {
    throw std::runtime_error("There is no property " + logname +
                             " in the workspace.");
  }

  Kernel::TimeSeriesProperty<double> *slog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          inputWS->run().getLogData(logname));
  if (!slog) {
    throw std::runtime_error("Problem reading property " + logname);
  }
  Kernel::TimeSeriesProperty<double> *pclog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          inputWS->run().getLogData("proton_charge"));
  if (!pclog) {
    throw std::runtime_error("Problem reading the proton charge property");
  }

  double average(0), error(0), protoncharge(0);
  double diffSeconds =
      static_cast<double>(
          (slog->firstTime() - pclog->firstTime()).total_nanoseconds()) *
      1e-9;
  if (getProperty("FixZero")) {
    diffSeconds = 0.;
  }
  std::vector<double> stime = slog->timesAsVectorSeconds();
  std::vector<double> svalue = slog->valuesAsVector();
  std::vector<double> pctime = pclog->timesAsVectorSeconds();
  std::vector<double> pcvalue = pclog->valuesAsVector();

  stime.push_back(EMPTY_DBL());
  svalue.push_back(0.0);
  pctime.push_back(EMPTY_DBL() * 1.1); // larger than stime
  pcvalue.push_back(0.0);

  std::vector<double>::iterator istime = stime.begin(),
                                isvalue = svalue.begin(),
                                ipctime = pctime.begin(),
                                ipcvalue = pcvalue.begin();

  for (; istime < (--stime.end()); ++istime) {
    // ignore all proton pulses before the lowest time for the log
    while ((*ipctime) < (*istime) + diffSeconds) {
      ++ipctime;
      ++ipcvalue;
    }
    // add together proton pulses before the current log time and the next log
    // time
    while ((*ipctime) < (*(istime + 1)) + diffSeconds) {
      protoncharge += (*ipcvalue);
      average += (*ipcvalue) * (*isvalue);
      error += (*ipcvalue) * (*isvalue) * (*isvalue);
      ++ipctime;
      ++ipcvalue;
    }
    ++isvalue;
  }

  if (protoncharge != 0) {
    g_log.warning()
        << "Proton charge is 0. Average and standard deviations are NANs"
        << std::endl;
  }
  g_log.debug() << "Sum = " << average << std::endl << "Sum squares = " << error
                << std::endl << "PC = " << protoncharge << std::endl;
  average /= protoncharge;
  error /= protoncharge;
  error = std::sqrt(std::fabs(error - average * average));

  g_log.information() << "Average value of " << logname << " is " << average
                      << " +/- " << error << std::endl;
  setProperty("Average", average);
  setProperty("Error", error);
}

} // namespace Algorithms
} // namespace Mantid
