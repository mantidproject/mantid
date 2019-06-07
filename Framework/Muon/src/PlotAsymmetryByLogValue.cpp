// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <cmath>
#include <vector>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMuon/PlotAsymmetryByLogValue.h"
#include "Poco/File.h"

using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
namespace // anonymous
{

/**
 * Convert a log property to a double value.
 *
 * @param property :: Pointer to a TimeSeriesProperty.
 * @param value :: Returned double value.
 * @return :: True if successful
 */
template <typename T>
bool convertLogToDouble(const Mantid::Kernel::Property *property, double &value,
                        const std::string &function) {
  const Mantid::Kernel::TimeSeriesProperty<T> *log =
      dynamic_cast<const Mantid::Kernel::TimeSeriesProperty<T> *>(property);
  if (log) {
    if (function == "Mean") {
      value = static_cast<double>(log->timeAverageValue());
    } else if (function == "First") {
      value = static_cast<double>(log->firstValue());
    } else if (function == "Min") {
      value = static_cast<double>(log->minValue());
    } else if (function == "Max") {
      value = static_cast<double>(log->maxValue());
    } else { // Default
      value = static_cast<double>(log->lastValue());
    }
    return true;
  }
  auto tlog =
      dynamic_cast<const Mantid::Kernel::PropertyWithValue<T> *>(property);
  if (tlog) {
    value = static_cast<double>(*tlog);
    return true;
  }
  return false;
}

} // namespace

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PlotAsymmetryByLogValue)

PlotAsymmetryByLogValue::PlotAsymmetryByLogValue()
    : Algorithm(), m_filenameBase(), m_filenameExt(), m_filenameZeros(),
      m_dtcType(), m_dtcFile(), m_forward_list(), m_backward_list(),
      m_int(true), m_red(-1), m_green(-1), m_minTime(-1.0), m_maxTime(-1.0),
      m_logName(), m_logFunc(), m_logValue(), m_redY(), m_redE(), m_greenY(),
      m_greenE(), m_sumY(), m_sumE(), m_diffY(), m_diffE(),
      m_allProperties("default"), m_currResName("__PABLV_results"),
      m_firstStart_ns(0) {}

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PlotAsymmetryByLogValue::init() {
  std::string nexusExt(".nxs");

  declareProperty(std::make_unique<FileProperty>("FirstRun", "",
                                                 FileProperty::Load, nexusExt),
                  "The name of the first workspace in the series.");
  declareProperty(std::make_unique<FileProperty>("LastRun", "",
                                                 FileProperty::Load, nexusExt),
                  "The name of the last workspace in the series.");
  declareProperty(
      std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                            Direction::Output),
      "The name of the output workspace containing the resulting asymmetries.");
  declareProperty("LogValue", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the log values which will be used as the x-axis "
                  "in the output workspace.");

  std::vector<std::string> optionsLog{"Mean", "Min", "Max", "First", "Last"};
  declareProperty(
      "Function", "Last", boost::make_shared<StringListValidator>(optionsLog),
      "The function to apply: 'Mean', 'Min', 'Max', 'First' or 'Last'.");

  declareProperty("Red", 1, "The period number for the 'red' data.");
  declareProperty("Green", EMPTY_INT(),
                  "The period number for the 'green' data.");

  std::vector<std::string> options{"Integral", "Differential"};
  declareProperty("Type", "Integral",
                  boost::make_shared<StringListValidator>(options),
                  "The calculation type: 'Integral' or 'Differential'.");
  declareProperty(
      "TimeMin", EMPTY_DBL(),
      "The beginning of the time interval used in the calculations.");
  declareProperty("TimeMax", EMPTY_DBL(),
                  "The end of the time interval used in the calculations.");

  declareProperty(std::make_unique<ArrayProperty<int>>("ForwardSpectra"),
                  "The list of spectra for the forward group. If not specified "
                  "the following happens. The data will be grouped according "
                  "to grouping information in the data, if available. The "
                  "forward will use the first of these groups.");
  declareProperty(std::make_unique<ArrayProperty<int>>("BackwardSpectra"),
                  "The list of spectra for the backward group. If not "
                  "specified the following happens. The data will be grouped "
                  "according to grouping information in the data, if "
                  "available. The backward will use the second of these "
                  "groups.");

  std::vector<std::string> deadTimeCorrTypes{"None", "FromRunData",
                                             "FromSpecifiedFile"};

  declareProperty("DeadTimeCorrType", deadTimeCorrTypes[0],
                  boost::make_shared<StringListValidator>(deadTimeCorrTypes),
                  "Type of Dead Time Correction to apply.");

  declareProperty(std::make_unique<FileProperty>("DeadTimeCorrFile", "",
                                                 FileProperty::OptionalLoad,
                                                 nexusExt),
                  "Custom file with Dead Times. Will be used only if "
                  "appropriate DeadTimeCorrType is set.");
}

/**
 *   Executes the algorithm
 */
void PlotAsymmetryByLogValue::exec() {

  // Check input properties to decide whether or not we can reuse previous
  // results, if any
  size_t is, ie;
  checkProperties(is, ie);

  Progress progress(this, 0, 1, ie - is + 1);

  // Loop through runs
  for (size_t i = is; i <= ie; i++) {

    // Check if run i was already loaded
    std::ostringstream logMessage;
    if (m_logValue.count(i)) {
      logMessage << "Found run " << i;
    } else {
      // Load run, apply dead time corrections and detector grouping
      Workspace_sptr loadedWs = doLoad(i);

      if (loadedWs) {
        // Analyse loadedWs
        doAnalysis(loadedWs, i);
      }
      logMessage << "Loaded run " << i;
    }
    progress.report(logMessage.str());
  }

  // Create the 2D workspace for the output
  int nplots = !m_greenY.empty() ? 4 : 1;
  size_t npoints = m_logValue.size();
  MatrixWorkspace_sptr outWS = create<Workspace2D>(
      nplots,         //  the number of plots
      Points(npoints) //  the number of data points on a plot
  );
  // Populate output workspace with data
  populateOutputWorkspace(outWS, nplots);
  // Assign the result to the output workspace property
  setProperty("OutputWorkspace", outWS);

  outWS = create<Workspace2D>(nplots + 1, Points(npoints));
  // Populate ws holding current results
  saveResultsToADS(outWS, nplots + 1);
}

/**  Checks input properties and compares them to previous values
 *   @param is :: [output] Number of the first run
 *   @param ie :: [output] Number of the last run
 */
void PlotAsymmetryByLogValue::checkProperties(size_t &is, size_t &ie) {

  // Log Value
  m_logName = getPropertyValue("LogValue");
  // Get function to apply to logValue
  m_logFunc = getPropertyValue("Function");
  // Get type of computation
  m_int = (getPropertyValue("Type") == "Integral");
  // Get grouping properties
  m_forward_list = getProperty("ForwardSpectra");
  m_backward_list = getProperty("BackwardSpectra");
  // Get green and red periods
  m_red = getProperty("Red");
  m_green = getProperty("Green");
  // Get time min and time max
  m_minTime = getProperty("TimeMin");
  m_maxTime = getProperty("TimeMax");
  // Get type of dead-time corrections
  m_dtcType = getPropertyValue("DeadTimeCorrType");
  m_dtcFile = getPropertyValue("DeadTimeCorrFile");
  // Get runs
  std::string firstFN = getProperty("FirstRun");
  std::string lastFN = getProperty("LastRun");

  // Parse run names and get the number of runs
  parseRunNames(firstFN, lastFN, m_filenameBase, m_filenameExt,
                m_filenameZeros);
  is = std::stoul(firstFN); // starting run number
  ie = std::stoul(lastFN);  // last run number
  if (ie < is) {
    throw std::runtime_error(
        "First run number is greater than last run number");
  }

  // Create a string holding all the properties
  std::ostringstream ss;
  ss << m_filenameBase << "," << m_filenameExt << "," << m_filenameZeros << ",";
  ss << m_dtcType << "," << m_dtcFile << ",";
  ss << getPropertyValue("ForwardSpectra") << ","
     << getPropertyValue("BackwardSpectra") << ",";
  ss << m_int << "," << m_minTime << "," << m_maxTime << ",";
  ss << m_red << "," << m_green << ",";
  ss << m_logName << ", " << m_logFunc;
  m_allProperties = ss.str();

  // Check if we can re-use results from previous run
  // We can reuse results if:
  // 1. There is a ws in the ADS with name m_currResName
  // 2. It is a MatrixWorkspace
  // 3. It has a title equatl to m_allProperties
  // This ws stores previous results as described below
  if (AnalysisDataService::Instance().doesExist(m_currResName)) {
    MatrixWorkspace_sptr prevResults =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_currResName);
    if (prevResults) {
      if (m_allProperties == prevResults->getTitle()) {
        // We can re-use results
        size_t nPoints = prevResults->blocksize();
        size_t nHisto = prevResults->getNumberHistograms();

        if (nHisto == 2) {
          // Only 'red' data
          for (size_t i = 0; i < nPoints; i++) {
            // The first spectrum contains: X -> run number, Y -> log value
            // The second spectrum contains: Y -> redY, E -> redE
            size_t run = static_cast<size_t>(prevResults->x(0)[i]);
            if ((run >= is) && (run <= ie)) {
              m_logValue[run] = prevResults->y(0)[i];
              m_redY[run] = prevResults->y(1)[i];
              m_redE[run] = prevResults->e(1)[i];
            }
          }
        } else {
          // 'Red' and 'Green' data
          for (size_t i = 0; i < nPoints; i++) {
            // The first spectrum contains: X -> run number, Y -> log value
            // The second spectrum contains: Y -> diffY, E -> diffE
            // The third spectrum contains: Y -> redY, E -> redE
            // The fourth spectrum contains: Y -> greenY, E -> greeE
            // The fifth spectrum contains: Y -> sumY, E -> sumE
            size_t run = static_cast<size_t>(prevResults->x(0)[i]);
            if ((run >= is) && (run <= ie)) {
              m_logValue[run] = prevResults->y(0)[i];
              m_diffY[run] = prevResults->y(1)[i];
              m_diffE[run] = prevResults->e(1)[i];
              m_redY[run] = prevResults->y(2)[i];
              m_redE[run] = prevResults->e(2)[i];
              m_greenY[run] = prevResults->y(3)[i];
              m_greenE[run] = prevResults->e(3)[i];
              m_sumY[run] = prevResults->y(4)[i];
              m_sumE[run] = prevResults->e(4)[i];
            }
          }
        }
      }
    }
  }
}

/**  Loads one run and applies dead-time corrections and detector grouping if
 * required
 *   @param runNumber :: [input] Run number specifying run to load
 *   @return :: Loaded workspace
 */
Workspace_sptr PlotAsymmetryByLogValue::doLoad(size_t runNumber) {

  // Get complete run name
  std::ostringstream fn, fnn;
  fnn << std::setw(m_filenameZeros) << std::setfill('0') << runNumber;
  fn << m_filenameBase << fnn.str() << m_filenameExt;

  // Check if file exists
  if (!Poco::File(fn.str()).exists()) {
    m_log.warning() << "File " << fn.str() << " not found\n";
    return Workspace_sptr();
  }

  // Load run
  IAlgorithm_sptr load = createChildAlgorithm("LoadMuonNexus");
  load->setPropertyValue("Filename", fn.str());
  load->setPropertyValue("DetectorGroupingTable", "detGroupTable");
  load->setPropertyValue("DeadTimeTable", "deadTimeTable");
  load->execute();
  Workspace_sptr loadedWs = load->getProperty("OutputWorkspace");

  // Check if dead-time corrections have to be applied
  if (m_dtcType != "None") {

    Workspace_sptr deadTimes;

    if (m_dtcType == "FromSpecifiedFile") {
      // Load corrections from file
      deadTimes = loadCorrectionsFromFile(m_dtcFile);
    } else {
      // Load corrections from run
      deadTimes = load->getProperty("DeadTimeTable");
    }
    if (!deadTimes) {
      throw std::runtime_error("Couldn't load dead times");
    }
    applyDeadtimeCorr(loadedWs, deadTimes);
  }

  // Group detectors
  Workspace_sptr grouping;
  if (m_forward_list.empty() && m_backward_list.empty()) {
    // Auto group
    grouping = load->getProperty("DetectorGroupingTable");
  } else {
    // Custom grouping
    grouping = createCustomGrouping(m_forward_list, m_backward_list);
  }
  if (!grouping)
    throw std::runtime_error("Couldn't load detector grouping");

  // Apply grouping
  groupDetectors(loadedWs, grouping);

  return loadedWs;
}

/**  Load dead-time corrections from specified file
 *   @param deadTimeFile :: [input] File to read corrections from
 *   @return :: Deadtime corrections loaded from file
 */
Workspace_sptr PlotAsymmetryByLogValue::loadCorrectionsFromFile(
    const std::string &deadTimeFile) {

  IAlgorithm_sptr alg = createChildAlgorithm("LoadNexusProcessed");
  alg->setPropertyValue("Filename", deadTimeFile);
  alg->setLogging(false);
  alg->execute();
  Workspace_sptr deadTimes = alg->getProperty("OutputWorkspace");
  return deadTimes;
}

/**  Populate output workspace with results
 *   @param outWS :: [input/output] Output workspace to populate
 *   @param nplots :: [input] Number of histograms
 */
void PlotAsymmetryByLogValue::populateOutputWorkspace(
    MatrixWorkspace_sptr &outWS, int nplots) {

  auto tAxis = new TextAxis(nplots);
  if (nplots == 1) {
    size_t i = 0;
    for (auto &value : m_logValue) {
      outWS->mutableX(0)[i] = value.second;
      outWS->mutableY(0)[i] = m_redY[value.first];
      outWS->mutableE(0)[i] = m_redE[value.first];
      i++;
    }
    tAxis->setLabel(0, "Asymmetry");

  } else {
    size_t i = 0;
    for (auto &value : m_logValue) {
      outWS->mutableX(0)[i] = value.second;
      outWS->mutableY(0)[i] = m_diffY[value.first];
      outWS->mutableE(0)[i] = m_diffE[value.first];
      outWS->mutableX(1)[i] = value.second;
      outWS->mutableY(1)[i] = m_redY[value.first];
      outWS->mutableE(1)[i] = m_redE[value.first];
      outWS->mutableX(2)[i] = value.second;
      outWS->mutableY(2)[i] = m_greenY[value.first];
      outWS->mutableE(2)[i] = m_greenE[value.first];
      outWS->mutableX(3)[i] = value.second;
      outWS->mutableY(3)[i] = m_sumY[value.first];
      outWS->mutableE(3)[i] = m_sumE[value.first];
      i++;
    }
    tAxis->setLabel(0, "Red-Green");
    tAxis->setLabel(1, "Red");
    tAxis->setLabel(2, "Green");
    tAxis->setLabel(3, "Red+Green");
  }
  outWS->replaceAxis(1, tAxis);
  outWS->getAxis(0)->title() = m_logName;
  outWS->setYUnitLabel("Asymmetry");
}

/**  Populate output workspace with results
 *   @param outWS :: [input/output] Output workspace to populate
 *   @param nplots :: [input] Number of histograms
 */
void PlotAsymmetryByLogValue::saveResultsToADS(MatrixWorkspace_sptr &outWS,
                                               int nplots) {

  if (nplots == 2) {
    size_t i = 0;
    for (auto &value : m_logValue) {
      size_t run = value.first;
      outWS->mutableX(0)[i] = static_cast<double>(run); // run number
      outWS->mutableY(0)[i] = value.second;             // log value
      outWS->mutableY(1)[i] = m_redY[run];              // redY
      outWS->mutableE(1)[i] = m_redE[run];              // redE
      i++;
    }
  } else {
    size_t i = 0;
    for (auto &value : m_logValue) {
      size_t run = value.first;
      outWS->mutableX(0)[i] = static_cast<double>(run); // run number
      outWS->mutableY(0)[i] = value.second;             // log value
      outWS->mutableY(1)[i] = m_diffY[run];             // diffY
      outWS->mutableE(1)[i] = m_diffE[run];             // diffE
      outWS->mutableY(2)[i] = m_redY[run];              // redY
      outWS->mutableE(2)[i] = m_redE[run];              // redE
      outWS->mutableY(3)[i] = m_greenY[run];            // greenY
      outWS->mutableE(3)[i] = m_greenE[run];            // greenE
      outWS->mutableY(4)[i] = m_sumY[run];              // sumY
      outWS->mutableE(4)[i] = m_sumE[run];              // sumE
      i++;
    }
  }
  // Set the title!
  outWS->setTitle(m_allProperties);

  // Save results to ADS
  // We can't set an output property to store the results as this algorithm
  // is executed as a child algorithm in the Muon ALC interface
  // If current results were saved as a property we couln't used
  // the functionality to re-use previous results in ALC
  AnalysisDataService::Instance().addOrReplace(m_currResName, outWS);
}

/**  Parse run names
 *   @param firstFN :: [input/output] First run's name
 *   @param lastFN :: [input/output] Last run's name
 *   @param fnBase :: [output] Runs base name
 *   @param fnExt :: [output] Runs extension
 *   @param fnZeros :: [output] Number of zeros in run's name
 */
void PlotAsymmetryByLogValue::parseRunNames(std::string &firstFN,
                                            std::string &lastFN,
                                            std::string &fnBase,
                                            std::string &fnExt, int &fnZeros) {

  // Parse first run's name
  std::string firstExt = firstFN.substr(firstFN.find_last_of('.'));
  firstFN.erase(firstFN.size() - 4);

  std::string firstBase = firstFN;
  size_t i = firstBase.size() - 1;
  while (isdigit(firstBase[i]))
    i--;
  if (i == firstBase.size() - 1) {
    throw Exception::FileError("File name must end with a number.", firstFN);
  }
  firstBase.erase(i + 1);
  firstFN.erase(0, firstBase.size());

  // Parse last run's name
  std::string lastExt = lastFN.substr(lastFN.find_last_of('.'));
  lastFN.erase(lastFN.size() - 4);

  std::string lastBase = lastFN;
  i = lastBase.size() - 1;
  while (isdigit(lastBase[i]))
    i--;
  if (i == lastBase.size() - 1) {
    throw Exception::FileError("File name must end with a number.", lastFN);
  }
  lastBase.erase(i + 1);
  lastFN.erase(0, lastBase.size());

  // Compare first and last
  if (firstBase != lastBase) {
    // Runs are not in the same directory

    // First run number with last base name
    std::ostringstream tempFirst;
    tempFirst << lastBase << firstFN << firstExt << '\n';
    std::string pathFirst = FileFinder::Instance().getFullPath(tempFirst.str());
    // Last run number with first base name
    std::ostringstream tempLast;
    tempLast << firstBase << lastFN << lastExt << '\n';
    std::string pathLast = FileFinder::Instance().getFullPath(tempLast.str());

    // Try to correct this on the fly by
    // checking if the last run can be found in the first directory...
    if (Poco::File(pathLast).exists()) {
      fnBase = firstBase;
      fnExt = firstExt;
      g_log.warning()
          << "First and last run are not in the same directory. File "
          << pathLast << " will be used instead.\n";
    } else if (Poco::File(pathFirst).exists()) {
      // ...or viceversa
      fnBase = lastBase;
      fnExt = lastExt;
      g_log.warning()
          << "First and last run are not in the same directory. File "
          << pathFirst << " will be used instead.\n";
    } else {
      throw std::runtime_error(
          "First and last runs are not in the same directory.");
    }

  } else {

    fnBase = firstBase;
    fnExt = firstExt;
  }
  fnZeros = static_cast<int>(firstFN.size());
}

/**  Apply dead-time corrections. The calculation is done by ApplyDeadTimeCorr
 * algorithm
 *   @param loadedWs :: [input/output] Workspace to apply corrections to
 *   @param deadTimes :: [input] Corrections to apply
 */
void PlotAsymmetryByLogValue::applyDeadtimeCorr(Workspace_sptr &loadedWs,
                                                Workspace_sptr deadTimes) {
  ScopedWorkspace ws(loadedWs);
  ScopedWorkspace dt(deadTimes);

  IAlgorithm_sptr applyCorr =
      AlgorithmManager::Instance().createUnmanaged("ApplyDeadTimeCorr");
  applyCorr->initialize();
  applyCorr->setLogging(false);
  applyCorr->setRethrows(true);
  applyCorr->setPropertyValue("InputWorkspace", ws.name());
  applyCorr->setPropertyValue("OutputWorkspace", ws.name());
  applyCorr->setProperty("DeadTimeTable", dt.name());
  applyCorr->execute();
  // Workspace should've been replaced in the ADS by ApplyDeadTimeCorr, so
  // need to
  // re-assign it
  loadedWs = ws.retrieve();
}

/** Creates grouping table from supplied forward and backward spectra
 * @param fwd :: [Input] Forward spectra
 * @param bwd :: [Input] Backward spectra
 * @return :: Workspace containing custom grouping
 */
Workspace_sptr
PlotAsymmetryByLogValue::createCustomGrouping(const std::vector<int> &fwd,
                                              const std::vector<int> &bwd) {

  ITableWorkspace_sptr group = boost::make_shared<TableWorkspace>();
  group->addColumn("vector_int", "group");
  TableRow row = group->appendRow();
  row << fwd;
  row = group->appendRow();
  row << bwd;

  return boost::dynamic_pointer_cast<Workspace>(group);
}

/**  Group detectors from table
 *   @param loadedWs :: [input/output] Workspace to apply grouping to
 *   @param grouping :: [input] Workspace containing grouping to apply
 */
void PlotAsymmetryByLogValue::groupDetectors(Workspace_sptr &loadedWs,
                                             Workspace_sptr grouping) {

  // Could be groups of workspaces, so need to work with ADS
  ScopedWorkspace inWS(loadedWs);
  ScopedWorkspace grWS(grouping);
  ScopedWorkspace outWS;

  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("MuonGroupDetectors");
  alg->initialize();
  alg->setLogging(false);
  alg->setPropertyValue("InputWorkspace", inWS.name());
  alg->setPropertyValue("DetectorGroupingTable", grWS.name());
  alg->setPropertyValue("OutputWorkspace", outWS.name());
  alg->execute();
  loadedWs = outWS.retrieve();
}

/**  Performs asymmetry analysis on a loaded workspace
 *   @param loadedWs :: [input] Workspace to apply analysis to
 *   @param index :: [input] Vector index where results will be stored
 */
void PlotAsymmetryByLogValue::doAnalysis(Workspace_sptr loadedWs,
                                         size_t index) {

  // Check if workspace is a workspace group
  WorkspaceGroup_sptr group =
      boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWs);

  // If it is not, we only have 'red' data
  if (!group) {
    MatrixWorkspace_sptr ws_red =
        boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWs);

    double Y, E;
    calcIntAsymmetry(ws_red, Y, E);
    m_logValue[index] = getLogValue(*ws_red);
    m_redY[index] = Y;
    m_redE[index] = E;

  } else {
    // It is a group

    // Process red data
    MatrixWorkspace_sptr ws_red;
    try {
      ws_red = boost::dynamic_pointer_cast<MatrixWorkspace>(
          group->getItem(m_red - 1));
    } catch (std::out_of_range &) {
      throw std::out_of_range("Red period out of range");
    }
    double YR, ER;
    calcIntAsymmetry(ws_red, YR, ER);
    double logValue = getLogValue(*ws_red);
    m_logValue[index] = logValue;
    m_redY[index] = YR;
    m_redE[index] = ER;

    if (m_green != EMPTY_INT()) {
      // Process green period if supplied by user
      MatrixWorkspace_sptr ws_green;
      try {
        ws_green = boost::dynamic_pointer_cast<MatrixWorkspace>(
            group->getItem(m_green - 1));
      } catch (std::out_of_range &) {
        throw std::out_of_range("Green period out of range");
      }
      double YG, EG;
      calcIntAsymmetry(ws_green, YG, EG);
      // Red data
      m_redY[index] = YR;
      m_redE[index] = ER;
      // Green data
      m_greenY[index] = YG;
      m_greenE[index] = EG;
      // Sum
      m_sumY[index] = YR + YG;
      m_sumE[index] = sqrt(ER * ER + EG * EG);
      // Diff
      calcIntAsymmetry(ws_red, ws_green, YR, ER);
      m_diffY[index] = YR;
      m_diffE[index] = ER;
    }
  } // else loadedGroup
}

/**  Calculate the integral asymmetry for a workspace.
 *   The calculation is done by AsymmetryCalc and Integration algorithms.
 *   @param ws :: The workspace
 *   @param Y :: Reference to a variable receiving the value of asymmetry
 *   @param E :: Reference to a variable receiving the value of the error
 */
void PlotAsymmetryByLogValue::calcIntAsymmetry(MatrixWorkspace_sptr ws,
                                               double &Y, double &E) {

  // Output workspace
  MatrixWorkspace_sptr out;

  if (!m_int) { //  "Differential asymmetry"
    IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
    asym->setLogging(false);
    asym->setProperty("InputWorkspace", ws);
    asym->execute();
    MatrixWorkspace_sptr asymWS = asym->getProperty("OutputWorkspace");

    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setLogging(false);
    integr->setProperty("InputWorkspace", asymWS);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    out = integr->getProperty("OutputWorkspace");

  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setLogging(false);
    integr->setProperty("InputWorkspace", ws);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr intWS = integr->getProperty("OutputWorkspace");

    IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
    asym->setLogging(false);
    asym->setProperty("InputWorkspace", intWS);
    asym->execute();
    out = asym->getProperty("OutputWorkspace");
  }

  Y = out->y(0)[0];
  E = out->e(0)[0];
}

/**  Calculate the integral asymmetry for a pair of workspaces (red & green).
 *   @param ws_red :: The red workspace
 *   @param ws_green :: The green workspace
 *   @param Y :: Reference to a variable receiving the value of asymmetry
 *   @param E :: Reference to a variable receiving the value of the error
 */
void PlotAsymmetryByLogValue::calcIntAsymmetry(MatrixWorkspace_sptr ws_red,
                                               MatrixWorkspace_sptr ws_green,
                                               double &Y, double &E) {
  if (!m_int) { //  "Differential asymmetry"
    HistogramBuilder builder;
    builder.setX(ws_red->x(0).size());
    builder.setY(ws_red->y(0).size());
    builder.setDistribution(ws_red->isDistribution());
    MatrixWorkspace_sptr tmpWS =
        create<MatrixWorkspace>(*ws_red, 1, builder.build());
    for (size_t i = 0; i < tmpWS->y(0).size(); i++) {
      double FNORM = ws_green->y(0)[i] + ws_red->y(0)[i];
      FNORM = FNORM != 0.0 ? 1.0 / FNORM : 1.0;
      double BNORM = ws_green->y(1)[i] + ws_red->y(1)[i];
      BNORM = BNORM != 0.0 ? 1.0 / BNORM : 1.0;
      double ZF = (ws_green->y(0)[i] - ws_red->y(0)[i]) * FNORM;
      double ZB = (ws_green->y(1)[i] - ws_red->y(1)[i]) * BNORM;
      tmpWS->mutableY(0)[i] = ZB - ZF;
      tmpWS->mutableE(0)[i] = (1.0 + ZF * ZF) * FNORM + (1.0 + ZB * ZB) * BNORM;
    }

    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", tmpWS);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->y(0)[0] / static_cast<double>(tmpWS->y(0).size());
    E = out->e(0)[0] / static_cast<double>(tmpWS->y(0).size());
  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws_red);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr intWS_red = integr->getProperty("OutputWorkspace");

    integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws_green);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr intWS_green = integr->getProperty("OutputWorkspace");

    double YIF = (intWS_green->y(0)[0] - intWS_red->y(0)[0]) /
                 (intWS_green->y(0)[0] + intWS_red->y(0)[0]);
    double YIB = (intWS_green->y(1)[0] - intWS_red->y(1)[0]) /
                 (intWS_green->y(1)[0] + intWS_red->y(1)[0]);

    Y = YIB - YIF;

    double VARIF =
        (1.0 + YIF * YIF) / (intWS_green->y(0)[0] + intWS_red->y(0)[0]);
    double VARIB =
        (1.0 + YIB * YIB) / (intWS_green->y(1)[0] + intWS_red->y(1)[0]);

    E = sqrt(VARIF + VARIB);
  }
}

/**
 * Get log value from a workspace. Convert to double if possible.
 *
 * @param ws :: [Input] The input workspace.
 * @return :: Log value.
 * @throw :: std::invalid_argument if the log cannot be converted to a double or
 *doesn't exist.
 */
double PlotAsymmetryByLogValue::getLogValue(MatrixWorkspace &ws) {

  const Run &run = ws.run();

  // Get the start & end time for the run
  Mantid::Types::Core::DateAndTime start, end;
  if (run.hasProperty("run_start") && run.hasProperty("run_end")) {
    start = run.getProperty("run_start")->value();
    end = run.getProperty("run_end")->value();
  }

  // If this is the first run, cache the start time
  if (m_firstStart_ns == 0) {
    m_firstStart_ns = start.totalNanoseconds();
  }

  // If the log asked for is the start or end time, we already have these.
  // Return it as a double in seconds, relative to start of first run
  constexpr static double nanosec_to_sec = 1.e-9;
  if (m_logName == "run_start") {
    return static_cast<double>(start.totalNanoseconds() - m_firstStart_ns) *
           nanosec_to_sec;
  } else if (m_logName == "run_end") {
    return static_cast<double>(end.totalNanoseconds() - m_firstStart_ns) *
           nanosec_to_sec;
  }

  // Otherwise, try converting the log value to a double
  auto *property = run.getLogData(m_logName);
  if (!property) {
    throw std::invalid_argument("Log " + m_logName + " does not exist.");
  }
  property->filterByTime(start, end);

  double value = 0;
  // try different property types
  if (convertLogToDouble<double>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<float>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<int32_t>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<int64_t>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<uint32_t>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<uint64_t>(property, value, m_logFunc))
    return value;
  // try if it's a string and can be lexically cast to double
  auto slog =
      dynamic_cast<const Mantid::Kernel::PropertyWithValue<std::string> *>(
          property);
  if (slog) {
    try {
      value = boost::lexical_cast<double>(slog->value());
      return value;
    } catch (std::exception &) {
      // do nothing, goto throw
    }
  }

  throw std::invalid_argument("Log " + m_logName +
                              " cannot be converted to a double type.");
}

} // namespace Algorithms
} // namespace Mantid
