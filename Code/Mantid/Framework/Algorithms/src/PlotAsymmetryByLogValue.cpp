//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>

#include "MantidAPI/FileProperty.h"
#include <MantidAPI/FileFinder.h>
#include "MantidAPI/Progress.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "Poco/File.h"

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

} // anonymous

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PlotAsymmetryByLogValue)

PlotAsymmetryByLogValue::PlotAsymmetryByLogValue()
    : Algorithm(), m_is(0), m_ie(0), m_filenameBase(), m_filenameExt(),
      m_filenameZeros(0), m_dtcType(), m_dtcFile(), m_forward_list(),
      m_backward_list(), m_int(), m_red(0), m_green(0), m_minTime(0.0),
      m_maxTime(0.0), m_logName(), m_logFunc() {}

/** Initialisation method. Declares properties to be used in algorithm.
*
*/
void PlotAsymmetryByLogValue::init() {
  std::string nexusExt(".nxs");

  declareProperty(
      new FileProperty("FirstRun", "", FileProperty::Load, nexusExt),
      "The name of the first workspace in the series.");
  declareProperty(new FileProperty("LastRun", "", FileProperty::Load, nexusExt),
                  "The name of the last workspace in the series.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the output workspace containing the resulting asymmetries.");
  declareProperty("LogValue", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the log values which will be used as the x-axis "
                  "in the output workspace.");

  std::vector<std::string> optionsLog;
  optionsLog.push_back("Mean");
  optionsLog.push_back("Min");
  optionsLog.push_back("Max");
  optionsLog.push_back("First");
  optionsLog.push_back("Last");
  declareProperty(
      "Function", "Last", boost::make_shared<StringListValidator>(optionsLog),
      "The function to apply: 'Mean', 'Min', 'Max', 'First' or 'Last'.");

  declareProperty("Red", 1, "The period number for the 'red' data.");
  declareProperty("Green", EMPTY_INT(),
                  "The period number for the 'green' data.");

  std::vector<std::string> options;
  options.push_back("Integral");
  options.push_back("Differential");
  declareProperty("Type", "Integral",
                  boost::make_shared<StringListValidator>(options),
                  "The calculation type: 'Integral' or 'Differential'.");
  declareProperty(
      "TimeMin", EMPTY_DBL(),
      "The beginning of the time interval used in the calculations.");
  declareProperty("TimeMax", EMPTY_DBL(),
                  "The end of the time interval used in the calculations.");

  declareProperty(new ArrayProperty<int>("ForwardSpectra"),
                  "The list of spectra for the forward group. If not specified "
                  "the following happens. The data will be grouped according "
                  "to grouping information in the data, if available. The "
                  "forward will use the first of these groups.");
  declareProperty(new ArrayProperty<int>("BackwardSpectra"),
                  "The list of spectra for the backward group. If not "
                  "specified the following happens. The data will be grouped "
                  "according to grouping information in the data, if "
                  "available. The backward will use the second of these "
                  "groups.");

  std::vector<std::string> deadTimeCorrTypes;
  deadTimeCorrTypes.push_back("None");
  deadTimeCorrTypes.push_back("FromRunData");
  deadTimeCorrTypes.push_back("FromSpecifiedFile");

  declareProperty("DeadTimeCorrType", deadTimeCorrTypes[0],
                  boost::make_shared<StringListValidator>(deadTimeCorrTypes),
                  "Type of Dead Time Correction to apply.");

  declareProperty(new FileProperty("DeadTimeCorrFile", "",
                                   FileProperty::OptionalLoad, nexusExt),
                  "Custom file with Dead Times. Will be used only if "
                  "appropriate DeadTimeCorrType is set.");
}

/**
*   Executes the algorithm
*/
void PlotAsymmetryByLogValue::exec() {

  // Check input properties to decide whether or not we can reuse previous
  // results, if any
  checkProperties();

  // Vectors to store results
  MantidVec logValue;
  MantidVec yRed, yGreen, yDiff, ySum;
  MantidVec eRed, eGreen, eDiff, eSum;

  Progress progress(this, 0, 1, m_ie - m_is + 1);

  // Loop through runs
  for (size_t i = m_is; i <= m_ie; i++) {

    // Load run, apply dead time corrections and detector grouping
    Workspace_sptr loadedWs = doLoad(i);

    if (loadedWs) {

      // Get Log value
      double x = getLogValue(loadedWs);
      logValue.push_back(x);

      // Analyse loadedWs
      std::vector<double> y, e;
      doAnalysis(loadedWs, y, e);

      yRed.push_back(y[0]);
      eRed.push_back(e[0]);

      if ((y.size() == 4) && (e.size() == 4)) {
        yGreen.push_back(y[1]);
        ySum.push_back(y[2]);
        yDiff.push_back(y[3]);
        eGreen.push_back(e[1]);
        eSum.push_back(e[2]);
        eDiff.push_back(e[3]);
      }
    }

    progress.report();
  }

  // Create the 2D workspace for the output
  size_t nplots = yGreen.empty() ? 1 : 4;
  size_t npoints = yRed.size();
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(
      "Workspace2D",
      nplots,  //  the number of plots
      npoints, //  the number of data points on a plot
      npoints  //  it's not a histogram
      );
  TextAxis *tAxis = new TextAxis(nplots);
  if (nplots == 1) {
    tAxis->setLabel(0, "Asymmetry");
    outWS->dataX(0) = logValue;
    outWS->dataY(0) = yRed;
    outWS->dataE(0) = eRed;
  } else {
    tAxis->setLabel(0, "Red-Green");
    tAxis->setLabel(1, "Red");
    tAxis->setLabel(2, "Green");
    tAxis->setLabel(3, "Red+Green");
    outWS->dataX(0) = logValue;
    outWS->dataY(0) = yDiff;
    outWS->dataE(0) = eDiff;
    outWS->dataX(1) = logValue;
    outWS->dataY(1) = yRed;
    outWS->dataE(1) = eRed;
    outWS->dataX(2) = logValue;
    outWS->dataY(2) = yGreen;
    outWS->dataE(2) = eGreen;
    outWS->dataX(3) = logValue;
    outWS->dataY(3) = ySum;
    outWS->dataE(3) = eSum;
  }
  outWS->replaceAxis(1, tAxis);
  outWS->getAxis(0)->title() = m_logName;
  outWS->setYUnitLabel("Asymmetry");
  setProperty("OutputWorkspace", outWS);
}

/**  Checks input properties and compares them to previous values
*/
void PlotAsymmetryByLogValue::checkProperties() {

  // Properties needed to select X axis
  // Log Value
  m_logName = getPropertyValue("LogValue");
  // Function to apply to logValue
  m_logFunc = getPropertyValue("Function");

  // Properties needed to calculate the asymmetry
  // Type of asymmetry
  m_int = (getPropertyValue("Type") == "Integral");
  // Get green and red periods
  m_red = getProperty("Red");
  m_green = getProperty("Green");
  // Get time min and time max
  m_minTime = getProperty("TimeMin");
  m_maxTime = getProperty("TimeMax");

  // Properties needed to load and pre-analyse the runs
  // Custom grouping
  m_forward_list = getProperty("ForwardSpectra");
  m_backward_list = getProperty("BackwardSpectra");
  // Type of dead-time corrections
  m_dtcType = getPropertyValue("DeadTimeCorrType");
  m_dtcFile = getPropertyValue("DeadTimeCorrFile");
  // First and last run in the set
  std::string firstFN = getProperty("FirstRun");
  std::string lastFN = getProperty("LastRun");

  // Parse run names and get the number of runs
  parseRunNames(firstFN, lastFN, m_filenameBase, m_filenameExt,
                m_filenameZeros);
  m_is = atoi(firstFN.c_str()); // starting run number
  m_ie = atoi(lastFN.c_str());  // last run number
  if (m_ie < m_is) {
    throw std::runtime_error(
        "First run number is greater than last run number");
  }
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
  std::string firstExt = firstFN.substr(firstFN.find_last_of("."));
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
  std::string lastExt = lastFN.substr(lastFN.find_last_of("."));
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
    tempFirst << lastBase << firstFN << firstExt << std::endl;
    std::string pathFirst = FileFinder::Instance().getFullPath(tempFirst.str());
    // Last run number with first base name
    std::ostringstream tempLast;
    tempLast << firstBase << lastFN << lastExt << std::endl;
    std::string pathLast = FileFinder::Instance().getFullPath(tempLast.str());

    // Try to correct this on the fly by
    // checking if the last run can be found in the first directory...
    if (Poco::File(pathLast).exists()) {
      fnBase = firstBase;
      fnExt = firstExt;
      g_log.warning()
          << "First and last run are not in the same directory. File "
          << pathLast << " will be used instead." << std::endl;
    } else if (Poco::File(pathFirst).exists()) {
      // ...or viceversa
      fnBase = lastBase;
      fnExt = lastExt;
      g_log.warning()
          << "First and last run are not in the same directory. File "
          << pathFirst << " will be used instead." << std::endl;
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

/**  Loads one run and applies dead-time corrections and detector grouping if
* required
*   @param runNumber :: [input] Run number specifying run to load
*/
Workspace_sptr PlotAsymmetryByLogValue::doLoad(int64_t runNumber) {

  // Get complete run name
  std::ostringstream fn, fnn;
  fnn << std::setw(m_filenameZeros) << std::setfill('0') << runNumber;
  fn << m_filenameBase << fnn.str() << m_filenameExt;

  // Check if file exists
  if (!Poco::File(fn.str()).exists()) {
    g_log.warning() << "File " << fn.str() << " not found" << std::endl;
    return Workspace_sptr();
  }

  // Load run
  IAlgorithm_sptr load = createChildAlgorithm("LoadMuonNexus");
  load->setPropertyValue("Filename", fn.str());
  load->execute();
  Workspace_sptr loadedWs = load->getProperty("OutputWorkspace");

  // Check if dead-time corrections have to be applied
  if (m_dtcType != "None") {

    Workspace_sptr deadTimes;

    if (m_dtcType == "FromSpecifiedFile") {
      // Load corrections from specified file
      deadTimes = loadCorrectionsFromFile(m_dtcFile);
    } else {
      // Load corrections from run
      deadTimes = load->getProperty("DeadTimeTable");
    }
    if (!deadTimes)
      throw std::runtime_error("Couldn't load dead times");

    // Apply corrections
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
*   @return :: Workspace containing loaded corrections
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

/**  Apply dead-time corrections. The calculation is done by ApplyDeadTimeCorr
* algorithm
*   @param loadedWs :: [input/output] Workspace to apply corrections to
*   @param deadTimes :: [input] Corrections to apply
*/
void PlotAsymmetryByLogValue::applyDeadtimeCorr(Workspace_sptr &loadedWs,
                                                Workspace_sptr deadTimes) {
  ScopedWorkspace ws(loadedWs);
  ScopedWorkspace dt(deadTimes);

  IAlgorithm_sptr applyCorr = createChildAlgorithm("ApplyDeadTimeCorr");
  applyCorr->setLogging(false);
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

  ITableWorkspace_sptr group =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
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

  IAlgorithm_sptr alg = createChildAlgorithm("MuonGroupDetectors");
  alg->setLogging(false);
  alg->setPropertyValue("InputWorkspace", inWS.name());
  alg->setPropertyValue("DetectorGroupingTable", grWS.name());
  alg->setPropertyValue("OutputWorkspace", outWS.name());
  alg->execute();

  loadedWs = outWS.retrieve();
}

/**  Performs asymmetry analysis on a given workspace
*   @param loadedWs :: [input] Workspace to apply analysis to
*   @param index :: [input] Vector index where results will be stored
*/
void PlotAsymmetryByLogValue::doAnalysis(Workspace_sptr loadedWs,
                                         std::vector<double> &y,
                                         std::vector<double> &e) {

  // Check if workspace is a workspace group
  WorkspaceGroup_sptr group =
      boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWs);

  if (!group) {
    // If it is not, we only have 'red' data

    MatrixWorkspace_sptr loadedWs2D =
        boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWs);

    double Y, E;
    calculateAsymmetry(loadedWs2D, Y, E);
    y.push_back(Y);
    e.push_back(E);

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
    calculateAsymmetry(ws_red, YR, ER);
    y.push_back(YR);
    e.push_back(ER);

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
      calculateAsymmetry(ws_green, YG, EG);
      y.push_back(YG);
      e.push_back(EG);
      y.push_back(YR + YG);
      e.push_back(sqrt(ER * ER + EG * EG));

      calculateAsymmetry(ws_red, ws_green, YR, ER);
      y.push_back(YR);
      e.push_back(ER);
    }
  }
}

/**  Calculate the integral asymmetry for a workspace. The calculation is done
* by AsymmetryCalc and Integration algorithms.
*   @param ws :: The workspace
*   @param Y :: [Output] Reference to a variable receiving the asymmetry
*   @param E :: [Output] Reference to a variable receiving the error
*/
void PlotAsymmetryByLogValue::calculateAsymmetry(MatrixWorkspace_sptr ws,
                                                 double &Y, double &E) {

  if (!m_int) {
    // "Differential" asymmetry:
    // Calculate asymmetry first, then integrate

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
    MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->readY(0)[0];
    E = out->readE(0)[0];

  } else {
    // "Integral" asymmetry:
    // Integrate first, then calculate asymmetry

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
    MatrixWorkspace_sptr out = asym->getProperty("OutputWorkspace");

    Y = out->readY(0)[0];
    E = out->readE(0)[0];
  }
}

/**  Calculate the integral asymmetry for a pair of workspaces (red & green).
*   @param ws_red :: [Input] The red workspace
*   @param ws_green :: [Input] The green workspace
*   @param Y :: [Output] Reference to a variable receiving the asymmetry
*   @param E :: [Output] Reference to a variable receiving the error
*/
void PlotAsymmetryByLogValue::calculateAsymmetry(MatrixWorkspace_sptr ws_red,
                                                 MatrixWorkspace_sptr ws_green,
                                                 double &Y, double &E) {

  if (!m_int) { //  "Differential asymmetry"

    MatrixWorkspace_sptr tmpWS = WorkspaceFactory::Instance().create(
        ws_red, 1, ws_red->readX(0).size(), ws_red->readY(0).size());

    for (size_t i = 0; i < tmpWS->dataY(0).size(); i++) {
      double FNORM = ws_green->readY(0)[i] + ws_red->readY(0)[i];
      FNORM = FNORM != 0.0 ? 1.0 / FNORM : 1.0;
      double BNORM = ws_green->readY(1)[i] + ws_red->readY(1)[i];
      BNORM = BNORM != 0.0 ? 1.0 / BNORM : 1.0;
      double ZF = (ws_green->readY(0)[i] - ws_red->readY(0)[i]) * FNORM;
      double ZB = (ws_green->readY(1)[i] - ws_red->readY(1)[i]) * BNORM;
      tmpWS->dataY(0)[i] = ZB - ZF;
      tmpWS->dataE(0)[i] = (1.0 + ZF * ZF) * FNORM + (1.0 + ZB * ZB) * BNORM;
    }

    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setLogging(false);
    integr->setProperty("InputWorkspace", tmpWS);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->readY(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
    E = out->readE(0)[0] / static_cast<double>(tmpWS->dataY(0).size());

  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setLogging(false);
    integr->setProperty("InputWorkspace", ws_red);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr intWS_red = integr->getProperty("OutputWorkspace");

    integr = createChildAlgorithm("Integration");
    integr->setLogging(false);
    integr->setProperty("InputWorkspace", ws_green);
    integr->setProperty("RangeLower", m_minTime);
    integr->setProperty("RangeUpper", m_maxTime);
    integr->execute();
    MatrixWorkspace_sptr intWS_green = integr->getProperty("OutputWorkspace");

    double YIF = (intWS_green->readY(0)[0] - intWS_red->readY(0)[0]) /
                 (intWS_green->readY(0)[0] + intWS_red->readY(0)[0]);
    double YIB = (intWS_green->readY(1)[0] - intWS_red->readY(1)[0]) /
                 (intWS_green->readY(1)[0] + intWS_red->readY(1)[0]);

    Y = YIB - YIF;

    double VARIF =
        (1.0 + YIF * YIF) / (intWS_green->readY(0)[0] + intWS_red->readY(0)[0]);
    double VARIB =
        (1.0 + YIB * YIB) / (intWS_green->readY(1)[0] + intWS_red->readY(1)[0]);

    E = sqrt(VARIF + VARIB);
  }
}

/**
 * Get log value from a workspace. Convert to double if possible.
 *
 * @param ws :: [Input] The input workspace.
 * @return :: Log value as double
 * @throw :: std::invalid_argument if the log cannot be converted to a double or
 *doesn't exist.
 */
double PlotAsymmetryByLogValue::getLogValue(Workspace_sptr ws) {

  MatrixWorkspace_sptr wsm;

  WorkspaceGroup_sptr wsg = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
  if (wsg) {
    wsm = boost::dynamic_pointer_cast<MatrixWorkspace>(wsg->getItem(0));
  } else {
    wsm = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
  }
  if (!wsm) {
    throw std::invalid_argument("Couldn't find run logs");
  }

  const Run &run = wsm->run();

  // Get the start & end time for the run
  Mantid::Kernel::DateAndTime start, end;
  if (run.hasProperty("run_start") && run.hasProperty("run_end")) {
    start = run.getProperty("run_start")->value();
    end = run.getProperty("run_end")->value();
  }

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
  if (convertLogToDouble<int>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<long>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<long long>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<unsigned int>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<unsigned long>(property, value, m_logFunc))
    return value;
  if (convertLogToDouble<unsigned long long>(property, value, m_logFunc))
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

} // namespace Algorithm
} // namespace Mantid
