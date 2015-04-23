//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "MantidAPI/FileProperty.h"
#include <MantidAPI/FileFinder.h>
#include "MantidAPI/Progress.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "Poco/File.h"

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

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
bool convertLogToDouble(const Mantid::Kernel::Property *property,
                        double &value, const std::string& function) {
  const Mantid::Kernel::TimeSeriesProperty<T> *log =
      dynamic_cast<const Mantid::Kernel::TimeSeriesProperty<T> *>(property);
  if (log) {
    if (function=="Mean") {
      value = static_cast<double>(log->timeAverageValue());
    } else if (function=="First") {
      value = static_cast<double>(log->firstValue());
    } else if (function=="Min") {
      value = static_cast<double>(log->minValue());
    } else if (function=="Max") {
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

// Static member variables
std::map<int64_t, double> PlotAsymmetryByLogValue::g_redX;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_redY;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_redE;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_greenX;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_greenY;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_greenE;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_sumX;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_sumY;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_sumE;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_diffX;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_diffY;
std::map<int64_t, double> PlotAsymmetryByLogValue::g_diffE;
std::string PlotAsymmetryByLogValue::g_logName;
std::string PlotAsymmetryByLogValue::g_logFunc;
std::string PlotAsymmetryByLogValue::g_stype;
std::vector<int> PlotAsymmetryByLogValue::g_forward_list;
std::vector<int> PlotAsymmetryByLogValue::g_backward_list;
int PlotAsymmetryByLogValue::g_red = 1;
int PlotAsymmetryByLogValue::g_green = EMPTY_INT();
std::string PlotAsymmetryByLogValue::g_dtcType;
std::string PlotAsymmetryByLogValue::g_dtcFile;
std::string PlotAsymmetryByLogValue::g_filenameBase;
std::string PlotAsymmetryByLogValue::g_filenameExt;
int PlotAsymmetryByLogValue::g_filenameZeros = 0;

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
  declareProperty("Function", "Last",
    boost::make_shared<StringListValidator>(optionsLog),
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
  size_t is, ie;
  checkProperties(is,ie);

  Progress progress(this, 0, 1, ie - is + 2);

  // Loop through runs
  for (size_t i = is; i <= ie; i++) {

    // Check if run i was already loaded
    if ( !g_redX.count(i) ) {

      // Load run, apply dead time corrections and detector grouping
      Workspace_sptr loadedWs = doLoad(i);

      // Analyse loadedWs
      doAnalysis (loadedWs, i);
    }

    progress.report();
  }


  // Create the 2D workspace for the output
  int nplots = (g_green!= EMPTY_INT()) ? 4 : 1;
  size_t npoints = ie - is + 1;
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(
      "Workspace2D",
      nplots,  //  the number of plots
      npoints, //  the number of data points on a plot
      npoints  //  it's not a histogram
      );
  // Populate output workspace with data
  populateOutputWorkspace(outWS,nplots);
  // Assign the result to the output workspace property
  setProperty("OutputWorkspace", outWS);

}

/**  Checks input properties and compares them to previous values
*/
void PlotAsymmetryByLogValue::checkProperties (size_t &is, size_t &ie) {

  // If any of the following properties has a different value from the
  // previous call, we need to re-do all the computations, which means
  // clearing static maps that store previous results

  // Log Value
  std::string logName = getPropertyValue("LogValue");
  // Get function to apply to logValue
  std::string logFunc = getPropertyValue("Function");
  // Get type of computation
  std::string stype = getPropertyValue("Type");
  // Get grouping properties
  std::vector<int> forward_list = getProperty("ForwardSpectra");
  std::vector<int> backward_list = getProperty("BackwardSpectra");
  // Get green and red periods
  int red = getProperty("Red");
  int green = getProperty("Green");
  // Get type of dead-time corrections
  std::string dtcType = getPropertyValue("DeadTimeCorrType");
  std::string dtcFile = getPropertyValue("DeadTimeCorrFile");
  // Get runs
  std::string firstFN = getProperty("FirstRun");
  std::string lastFN = getProperty("LastRun");

  // Parse run names and get the number of runs
  std::string filenameBase, filenameExt;
  int filenameZeros;
  parseRunNames( firstFN, lastFN, filenameBase, filenameExt, filenameZeros);
  is = atoi(firstFN.c_str()); // starting run number
  ie = atoi(lastFN.c_str());  // last run number

  if ( !g_redX.empty() ) {

    size_t isOld = g_redX.begin()->first; // Old first run number
    size_t ieOld = g_redX.rbegin()->first; // Old last run number

    // Check if any property has changed
    if ( g_logName != logName ||
      g_logFunc != logFunc ||
      g_stype != stype ||
      g_forward_list != forward_list ||
      g_backward_list != backward_list ||
      g_green != green ||
      g_red != red ||
      g_dtcType != dtcType ||
      g_dtcFile != dtcFile ||
      g_filenameBase != filenameBase ||
      g_filenameExt != filenameExt ||
      g_filenameZeros != filenameZeros) {

        // If so, clear previous results
        clearResultsFromTo(isOld,ieOld);

    } else {

      // If all of the above are the same, we may re-use previous
      // results, provided that new run numbers are 'appropriate'

      if ( is > ieOld || ie < isOld ) {

        // Completely new set of runs
        clearResultsFromTo(isOld,ieOld);

      } else {

        if ( is > isOld ) {

          // Remove runs from isOld to is-1
          clearResultsFromTo(isOld,is-1);
        }

        if ( ie < ieOld ) {

          // Remove runs from ie+1 to ieOld
          clearResultsFromTo(ie+1,ieOld);
        }

      }
    }
  }

  g_logName = logName;
  g_logFunc = logFunc;
  g_stype = stype;
  m_int = g_stype == "Integral";
  g_forward_list = forward_list;
  g_backward_list = backward_list;
  m_autogroup = (g_forward_list.size() == 0 && g_backward_list.size() == 0);
  g_green = green;
  g_red = red;
  g_dtcType = dtcType;
  g_dtcFile = dtcFile;
  g_filenameBase = filenameBase;
  g_filenameExt = filenameExt;
  g_filenameZeros = filenameZeros;


}

/**  Clears any possible result from previous call
*/
void PlotAsymmetryByLogValue::clearResultsFromTo(size_t is, size_t ie) {

  for (size_t i=is; i<=ie; i++) {
    g_redX.erase(i);
    g_redY.erase(i);
    g_redE.erase(i);
    g_greenX.erase(i);
    g_greenY.erase(i);
    g_greenE.erase(i);
    g_sumX.erase(i);
    g_sumY.erase(i);
    g_sumE.erase(i);
    g_diffX.erase(i);
    g_diffY.erase(i);
    g_diffE.erase(i);
  }

}

/**  Loads one run and applies dead-time corrections and detector grouping if required
*   @param runNumber :: [input] Run number specifying run to load
*/
Workspace_sptr PlotAsymmetryByLogValue::doLoad (int64_t runNumber ) {

  // Get complete run name
  std::ostringstream fn, fnn;
  fnn << std::setw(g_filenameZeros) << std::setfill('0') << runNumber;
  fn << g_filenameBase << fnn.str() << g_filenameExt;

  // Load run
  IAlgorithm_sptr load = createChildAlgorithm("LoadMuonNexus");
  load->setPropertyValue("Filename", fn.str());
  load->execute();
  Workspace_sptr loadedWs = load->getProperty("OutputWorkspace");

  // Check if dead-time corrections have to be applied
  if (g_dtcType != "None") {
    if (g_dtcType == "FromSpecifiedFile") {

      // If user specifies a file, load corrections now
      Workspace_sptr customDeadTimes;
      loadCorrectionsFromFile (customDeadTimes, g_dtcFile);
      applyDeadtimeCorr (loadedWs, customDeadTimes);
    } else {
      // Load corrections from run
      Workspace_sptr deadTimes = load->getProperty("DeadTimeTable");
      applyDeadtimeCorr (loadedWs, deadTimes);
    }
  }

  // If m_autogroup, group detectors
  if (m_autogroup) {
    Workspace_sptr loadedDetGrouping = load->getProperty("DetectorGroupingTable");
    if (!loadedDetGrouping)
      throw std::runtime_error("No grouping info in the file.\n\nPlease "
      "specify grouping manually");
    groupDetectors(loadedWs,loadedDetGrouping);
  }

  return loadedWs;
}

/**  Load dead-time corrections from specified file
*   @param customDeadTimes :: [input/output] Output workspace to store corrections
*   @param deadTimeFile :: [input] File to read corrections from
*/
void PlotAsymmetryByLogValue::loadCorrectionsFromFile (Workspace_sptr &customDeadTimes, std::string deadTimeFile )
{
  IAlgorithm_sptr loadDeadTimes = createChildAlgorithm("LoadNexusProcessed");
  loadDeadTimes->setPropertyValue("Filename", deadTimeFile);
  loadDeadTimes->setProperty("OutputWorkspace", customDeadTimes);
  loadDeadTimes->executeAsChildAlg();
  customDeadTimes = loadDeadTimes->getProperty("OutputWorkspace");
}
/**  Populate output workspace with results
*   @param outWS :: [input/output] Output workspace to populate
*   @param nplots :: [input] Number of histograms
*/
void PlotAsymmetryByLogValue::populateOutputWorkspace (MatrixWorkspace_sptr &outWS, int nplots)
{
  TextAxis *tAxis = new TextAxis(nplots);
  if (nplots == 1) {

    std::vector<double> vecRedX, vecRedY, vecRedE;
    for (auto it=g_redX.begin(); it!=g_redX.end(); ++it)
    {
      vecRedX.push_back( g_redX[ it->first ] );
      vecRedY.push_back( g_redY[ it->first ] );
      vecRedE.push_back( g_redE[ it->first ] );
    }

    tAxis->setLabel(0, "Asymmetry");
    outWS->dataX(0) = vecRedX;
    outWS->dataY(0) = vecRedY;
    outWS->dataE(0) = vecRedE;
  } else {

    std::vector<double> vecRedX, vecRedY, vecRedE;
    std::vector<double> vecGreenX, vecGreenY, vecGreenE;
    std::vector<double> vecSumX, vecSumY, vecSumE;
    std::vector<double> vecDiffX, vecDiffY, vecDiffE;
    for (auto it=g_redX.begin(); it!=g_redX.end(); ++it)
    {
      vecRedX.push_back( g_redX[ it->first ] );
      vecRedY.push_back( g_redY[ it->first ] );
      vecRedE.push_back( g_redE[ it->first ] );
      vecGreenX.push_back( g_greenX[ it->first ] );
      vecGreenY.push_back( g_greenY[ it->first ] );
      vecGreenE.push_back( g_greenE[ it->first ] );
      vecSumX.push_back( g_sumX[ it->first ] );
      vecSumY.push_back( g_sumY[ it->first ] );
      vecSumE.push_back( g_sumE[ it->first ] );
      vecDiffX.push_back( g_diffX[ it->first ] );
      vecDiffY.push_back( g_diffY[ it->first ] );
      vecDiffE.push_back( g_diffE[ it->first ] );
    }

    tAxis->setLabel(0, "Red-Green");
    tAxis->setLabel(1, "Red");
    tAxis->setLabel(2, "Green");
    tAxis->setLabel(3, "Red+Green");
    outWS->dataX(0) = vecDiffX;
    outWS->dataY(0) = vecDiffY;
    outWS->dataE(0) = vecDiffE;
    outWS->dataX(1) = vecRedX;
    outWS->dataY(1) = vecRedY;
    outWS->dataE(1) = vecRedE;
    outWS->dataX(2) = vecGreenX;
    outWS->dataY(2) = vecGreenY;
    outWS->dataE(2) = vecGreenE;
    outWS->dataX(3) = vecSumX;
    outWS->dataY(3) = vecSumY;
    outWS->dataE(3) = vecSumE;
  }
  outWS->replaceAxis(1, tAxis);
  outWS->getAxis(0)->title() = g_logName;
  outWS->setYUnitLabel("Asymmetry");
}
/**  Parse run names
*   @param firstFN :: [input/output] First run's name
*   @param lastFN :: [input/output] Last run's name
*   @param fnBase :: [output] Runs base name
*   @param fnExt :: [output] Runs extension
*   @param fnZeros :: [output] Number of zeros in run's name
*/
void PlotAsymmetryByLogValue::parseRunNames (std::string& firstFN, std::string& lastFN, std::string& fnBase, std::string& fnExt, int& fnZeros)
{

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
  if ( firstBase != lastBase ) {
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
    if ( Poco::File(pathLast).exists() ) {
      fnBase = firstBase;
      fnExt = firstExt;
      g_log.warning() << "First and last run are not in the same directory. File " 
        << pathLast << " will be used instead." << std::endl;
    } else if (Poco::File(pathFirst).exists()) {
      // ...or viceversa
      fnBase = lastBase;
      fnExt = lastExt;
      g_log.warning() << "First and last run are not in the same directory. File " 
        << pathFirst << " will be used instead." << std::endl;
    } else {
      throw std::runtime_error("First and last runs are not in the same directory.");
    }
   
  } else {

    fnBase = firstBase;
    fnExt = firstExt;
  }
  fnZeros = static_cast<int>(firstFN.size());

}

/**  Apply dead-time corrections. The calculation is done by ApplyDeadTimeCorr algorithm
*   @param loadedWs :: [input/output] Workspace to apply corrections to
*   @param deadTimes :: [input] Corrections to apply
*/
void PlotAsymmetryByLogValue::applyDeadtimeCorr (Workspace_sptr &loadedWs, Workspace_sptr deadTimes)
{
  ScopedWorkspace ws(loadedWs);
  ScopedWorkspace dt(deadTimes);

  IAlgorithm_sptr applyCorr = AlgorithmManager::Instance().create("ApplyDeadTimeCorr");
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

/**  Group detectors from specified file
*   @param loadedWs :: [input/output] Workspace to apply grouping to
*   @param loadedDetGrouping :: [input] Workspace storing detectors grouping
*/
void PlotAsymmetryByLogValue::groupDetectors (Workspace_sptr &loadedWs, Workspace_sptr loadedDetGrouping)
{

  // Could be groups of workspaces, so need to work with ADS
  ScopedWorkspace inWS(loadedWs);
  ScopedWorkspace grouping(loadedDetGrouping);
  ScopedWorkspace outWS;

  IAlgorithm_sptr applyGrouping = AlgorithmManager::Instance().create("MuonGroupDetectors");
  applyGrouping->setLogging(false);
  applyGrouping->setRethrows(true);

  applyGrouping->setPropertyValue("InputWorkspace", inWS.name());
  applyGrouping->setPropertyValue("DetectorGroupingTable", grouping.name());
  applyGrouping->setPropertyValue("OutputWorkspace", outWS.name());
  applyGrouping->execute();

  loadedWs = outWS.retrieve();
}

/**  Performs asymmetry analysis on a loaded workspace
*   @param loadedWs :: [input] Workspace to apply analysis to
*   @param index :: [input] Vector index where results will be stored
*/
void PlotAsymmetryByLogValue::doAnalysis (Workspace_sptr loadedWs, int64_t index ) {

    // Check if workspace is a workspace group
    WorkspaceGroup_sptr loadedGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWs);

    // If it is not, we only have 'red' data
    if (!loadedGroup) {
      Workspace2D_sptr loadedWs2D =
          boost::dynamic_pointer_cast<Workspace2D>(loadedWs);

      double Y, E;
      calcIntAsymmetry(loadedWs2D, Y, E);
      g_redX[index]=getLogValue(*loadedWs2D);
      g_redY[index]=Y;
      g_redE[index]=E;

    } else {

      DataObjects::Workspace2D_sptr ws_red;
      DataObjects::Workspace2D_sptr ws_green;
      // Run through the periods of the loaded file and save the
      // selected ones
      for (int mi = 0; mi < loadedGroup->getNumberOfEntries(); mi++) {

        Workspace2D_sptr memberWs =
            boost::dynamic_pointer_cast<Workspace2D>(loadedGroup->getItem(mi));
        int period = mi + 1;
        if ( period == g_red ){
          ws_red = memberWs;
        }
        if ( g_green!= EMPTY_INT() ){
          if ( period == g_green ){
            ws_green = memberWs;
          }
        }
      }

      // Check ws_red
      if (!ws_red){
        throw std::invalid_argument("Red period is out of range");
      }
      // Check ws_green
      if ( (g_green!=EMPTY_INT()) && (!ws_green) ){
        throw std::invalid_argument("Green period is out of range");
      }

      if ( g_green==EMPTY_INT() ){
        double Y, E;
        calcIntAsymmetry(ws_red, Y, E);
        g_redX[index] = getLogValue(*ws_red);
        g_redY[index] = Y;
        g_redE[index] = E;

      } else{
      
        double YR, ER;
        double YG, EG;
        double logValue = getLogValue(*ws_red);
        calcIntAsymmetry(ws_red, YR, ER);
        calcIntAsymmetry(ws_green, YG, EG);
        // Red data
        g_redX[index] = logValue;
        g_redY[index] = YR;
        g_redE[index] = ER;
        // Green data
        g_greenX[index] = logValue;
        g_greenY[index] = YG;
        g_greenE[index] = EG;
        // Sum
        g_sumX[index] = logValue;
        g_sumY[index] = YR+YG;
        g_sumE[index] = sqrt(ER * ER + EG * EG);
        // move to last for safety since some grouping takes place in the
        // calcIntAsymmetry call below
        calcIntAsymmetry(ws_red, ws_green, YR, ER);
        g_diffX[index] = logValue;
        g_diffY[index] = YR;
        g_diffE[index] = ER;
      }
    } // else loadedGroup

}

/**  Calculate the integral asymmetry for a workspace.
*   The calculation is done by MuonAsymmetryCalc and SimpleIntegration
* algorithms.
*   @param ws :: The workspace
*   @param Y :: Reference to a variable receiving the value of asymmetry
*   @param E :: Reference to a variable receiving the value of the error
*/
void PlotAsymmetryByLogValue::calcIntAsymmetry(API::MatrixWorkspace_sptr ws,
                                               double &Y, double &E) {
  Property *startXprop = getProperty("TimeMin");
  Property *endXprop = getProperty("TimeMax");
  bool setX = !startXprop->isDefault() && !endXprop->isDefault();
  double startX(0.0), endX(0.0);
  if (setX) {
    startX = getProperty("TimeMin");
    endX = getProperty("TimeMax");
  }
  if (!m_int) { //  "Differential asymmetry"
    IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
    asym->initialize();
    asym->setProperty("InputWorkspace", ws);
    asym->setPropertyValue("OutputWorkspace", "tmp");
    if (!m_autogroup) {
      asym->setProperty("ForwardSpectra", g_forward_list);
      asym->setProperty("BackwardSpectra", g_backward_list);
    }
    asym->execute();
    MatrixWorkspace_sptr asymWS = asym->getProperty("OutputWorkspace");

    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", asymWS);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->readY(0)[0];
    E = out->readE(0)[0];
  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr intWS = integr->getProperty("OutputWorkspace");

    IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
    asym->initialize();
    asym->setProperty("InputWorkspace", intWS);
    asym->setPropertyValue("OutputWorkspace", "tmp");
    if (!m_autogroup) {
      asym->setProperty("ForwardSpectra", g_forward_list);
      asym->setProperty("BackwardSpectra", g_backward_list);
    }
    asym->execute();
    MatrixWorkspace_sptr out = asym->getProperty("OutputWorkspace");

    Y = out->readY(0)[0];
    E = out->readE(0)[0];
  }
}

/**  Calculate the integral asymmetry for a workspace (red & green).
*   The calculation is done by MuonAsymmetryCalc and SimpleIntegration
* algorithms.
*   @param ws_red :: The red workspace
*   @param ws_green :: The green workspace
*   @param Y :: Reference to a variable receiving the value of asymmetry
*   @param E :: Reference to a variable receiving the value of the error
*/
void
PlotAsymmetryByLogValue::calcIntAsymmetry(API::MatrixWorkspace_sptr ws_red,
                                          API::MatrixWorkspace_sptr ws_green,
                                          double &Y, double &E) {
  if (!m_autogroup) {
    groupDetectors(ws_red, g_backward_list);
    groupDetectors(ws_red, g_forward_list);
    groupDetectors(ws_green, g_backward_list);
    groupDetectors(ws_green, g_forward_list);
  }

  Property *startXprop = getProperty("TimeMin");
  Property *endXprop = getProperty("TimeMax");
  bool setX = !startXprop->isDefault() && !endXprop->isDefault();
  double startX(0.0), endX(0.0);
  if (setX) {
    startX = getProperty("TimeMin");
    endX = getProperty("TimeMax");
  }
  if (!m_int) { //  "Differential asymmetry"

    API::MatrixWorkspace_sptr tmpWS = API::WorkspaceFactory::Instance().create(
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
    integr->setProperty("InputWorkspace", tmpWS);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->readY(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
    E = out->readE(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws_red);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr intWS_red =
        integr->getProperty("OutputWorkspace");

    integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws_green);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr intWS_green =
        integr->getProperty("OutputWorkspace");

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

/**  Group detectors in the workspace.
 *  @param ws :: A local workspace
 *  @param spectraList :: A list of spectra to group.
 */
void
PlotAsymmetryByLogValue::groupDetectors(API::MatrixWorkspace_sptr &ws,
                                        const std::vector<int> &spectraList) {
  API::IAlgorithm_sptr group = createChildAlgorithm("GroupDetectors");
  group->setProperty("InputWorkspace", ws);
  group->setProperty("SpectraList", spectraList);
  group->setProperty("KeepUngroupedSpectra", true);
  group->execute();
  ws = group->getProperty("OutputWorkspace");
}

/**
 * Get log value from a workspace. Convert to double if possible.
 *
 * @param ws :: The input workspace.
 * @return :: Log value.
 * @throw :: std::invalid_argument if the log cannot be converted to a double or
 *doesn't exist.
 */
double PlotAsymmetryByLogValue::getLogValue(MatrixWorkspace &ws) {
  auto *property = ws.run().getLogData(g_logName);
  if (!property) {
    throw std::invalid_argument("Log " + g_logName + " does not exist.");
  }
  double value = 0;
  // try different property types
  if (convertLogToDouble<double>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<float>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<int>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<long>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<long long>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<unsigned int>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<unsigned long>(property, value, g_logFunc))
    return value;
  if (convertLogToDouble<unsigned long long>(property, value, g_logFunc))
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

  throw std::invalid_argument("Log " + g_logName +
                              " cannot be converted to a double type.");
}



} // namespace Algorithm
} // namespace Mantid
