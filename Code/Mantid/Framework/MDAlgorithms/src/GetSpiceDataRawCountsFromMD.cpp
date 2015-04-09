#include "MantidMDAlgorithms/GetSpiceDataRawCountsFromMD.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/IMDIterator.h"

namespace Mantid
{
namespace MDAlgorithms
{

using namespace Mantid::API;
using namespace Mantid::Kernel;

DECLARE_ALGORITHM(GetSpiceDataRawCountsFromMD)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GetSpiceDataRawCountsFromMD::GetSpiceDataRawCountsFromMD() {}

//----------------------------------------------------------------------------------------------
/** Destructor
  */
GetSpiceDataRawCountsFromMD::~GetSpiceDataRawCountsFromMD() {}

//----------------------------------------------------------------------------------------------
/** Initialization
 * @brief GetSpiceDataRawCountsFromMD::init
 */
void GetSpiceDataRawCountsFromMD::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "Name of the input data MDEventWorkspace from which the raw "
                  "values are retrieved.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "MonitorWorkspace", "", Direction::Input),
                  "Name of the input monitor MDEventWorkspace paired with "
                  "input data workspace. ");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                             Direction::Output),
      "Name of the output MatrixWorkspace containing the raw data required.");

  std::vector<std::string> vecmode;
  vecmode.push_back("Pt.");
  vecmode.push_back("Detector");
  vecmode.push_back("Sample Log");
  auto modevalidator = boost::make_shared<ListValidator<std::string> >(vecmode);
  declareProperty(
      "Mode", "Detector", modevalidator,
      "Mode selector.  (1) Pt.: get the raw detectors' signal of the "
      "specified Pt.; "
      "(2) Detector: get one detector's signals along all experiment points; "
      "(3) Sample Log: get the value of a sample log along all experiment "
      "points.");

  declareProperty(
      "XLabel", "",
      "Label for the X-values of the output MatrixWorkspace. "
      "For mode 'Pt.', it won't take value.  The output of X-axis is always "
      "2-theta. "
      "For mode 'Detector', if it is left blank, "
      "the default then will be 2-theta value of detector's position. "
      "For mode 'Sample Log', the default value is 'Pt.'. "
      "In the later 2 modes, XLabel can be any supported sample log's name.");

  declareProperty(
      "Pt", EMPTY_INT(),
      "Experiment point number (i.e., run_number in MDEventWorkspace "
      "of the detectors' counts to be exported. "
      "It is used in mode 'Pt.' only. ");

  declareProperty("DetectorID", EMPTY_INT(),
                  "Detector ID of the detector whose raw counts "
                  "will be exported for all experiment points."
                  "It is used in mode 'Detector' only. ");

  declareProperty("SampleLogName", "",
                  "Name of the sample log whose value to be exported. "
                  "It is used in mode 'Sample Log' only. ");

  declareProperty("NormalizeByMonitorCounts", true,
                  "If specified as true, all the output detectors' counts will "
                  "be normalized by their monitor counts. ");
}

//----------------------------------------------------------------------------------------------
/** Execution body
 * @brief GetSpiceDataRawCountsFromMD::exec
 */
void GetSpiceDataRawCountsFromMD::exec() {
  // Process input parameters
  IMDEventWorkspace_sptr datamdws = getProperty("InputWorkspace");
  IMDEventWorkspace_sptr monitormdws = getProperty("MonitorWorkspace");
  std::string mode = getProperty("Mode");
  bool donormalize = getProperty("NormalizeByMonitorCounts");
  std::string xlabel = getProperty("XLabel");

  // Branch out by mode
  std::vector<double> vecX;
  std::vector<double> vecY;
  std::string ylabel("");
  if (mode.compare("Pt.") == 0) {
    // export detector counts for one specific Pt./run number
    int runnumber = getProperty("Pt");
    if (isEmpty(runnumber))
      throw std::runtime_error("For 'Pt.', value of 'Pt.' must be specified.");
    exportDetCountsOfRun(datamdws, monitormdws, runnumber, vecX, vecY, xlabel,
                         ylabel, donormalize);
  } else if (mode.compare("Detector") == 0) {
    int detid = getProperty("DetectorID");
    if (isEmpty(detid))
      throw std::runtime_error(
          "For mode 'Detector', value of 'DetectorID' must be specified.");
    exportIndividualDetCounts(datamdws, monitormdws, detid, vecX, vecY, xlabel,
                              ylabel, donormalize);
  } else if (mode.compare("Sample Log") == 0) {
    std::string samplelogname = getProperty("SampleLogName");
    if (samplelogname.size() == 0)
      throw std::runtime_error(
          "For mode 'Sample Log', value of 'SampleLogName' must be specified.");
    exportSampleLogValue(datamdws, samplelogname, vecX, vecY, xlabel, ylabel);
  } else {
    // Raise exception
    std::stringstream ess;
    ess << "Mode " << mode << " is not supported.";
    throw std::runtime_error(ess.str());
  }

  // Create and export output workspace
  MatrixWorkspace_sptr outws =
      createOutputWorkspace(vecX, vecY, xlabel, ylabel);
  setProperty("OutputWorkspace", outws);

  return;
}

//----------------------------------------------------------------------------------------------
/** Export all detectors' counts of a particular run
 * @brief GetSpiceDataRawCountsFromMD::exportDetCountsOfRun
 * @param datamdws
 * @param monitormdws
 * @param runnumber
 * @param vecX
 * @param vecY
 * @param xlabel
 * @param ylabel
 * @param donormalize
 */
void GetSpiceDataRawCountsFromMD::exportDetCountsOfRun(
    API::IMDEventWorkspace_const_sptr datamdws,
    API::IMDEventWorkspace_const_sptr monitormdws, const int runnumber,
    std::vector<double> &vecX, std::vector<double> &vecY, std::string &xlabel,
    std::string &ylabel, bool donormalize) {
  // Get detector counts
  std::vector<double> vec2theta;
  std::vector<double> vecDetCounts;
  int detid = -1;
  getDetCounts(datamdws, runnumber, detid, vec2theta, vecDetCounts, true);
  if (vec2theta.size() != vecDetCounts.size())
    throw std::runtime_error(
        "Logic error! Vector of 2theta must have same size as "
        "vector of detectors' counts.");

  // Get monitor counts
  std::vector<double> vec2thetaMon;
  std::vector<double> vecMonitorCounts;
  if (donormalize)
    getDetCounts(monitormdws, runnumber, detid, vec2thetaMon, vecMonitorCounts,
                 false);

  // Normalize if required
  if (donormalize) {
    // check
    if (vecDetCounts.size() != vecMonitorCounts.size())
      throw std::runtime_error(
          "Number of detectors' counts' is different from that of "
          "monitor counts.");

    for (size_t i = 0; i < vecDetCounts.size(); ++i)
      if (vecMonitorCounts[i] > 1.0E-9)
        vecDetCounts[i] = vecDetCounts[i] / vecMonitorCounts[i];
      else
        vecDetCounts[i] = 0.;
  }

  // Sort and output
  const size_t numdets = vecDetCounts.size();
  std::vector<std::pair<double, double> > vecPair(numdets);
  for (size_t i = 0; i < numdets; ++i) {
    vecPair[i] = std::make_pair(vec2theta[i], vecDetCounts[i]);
  }
  std::sort(vecPair.begin(), vecPair.end());

  // Apply to output
  vecX.resize(numdets);
  vecY.resize(numdets);
  for (size_t i = 0; i < numdets; ++i) {
    vecX[i] = vecPair[i].first;
    vecY[i] = vecPair[i].second;
  }

  // Set up label
  xlabel = "Degrees";
  if (donormalize)
    ylabel = "Noramlized Intensity";
  else
    ylabel = "Counts";
}

//----------------------------------------------------------------------------------------------
/** Export a detector's counts accross all runs
 * @brief GetSpiceDataRawCountsFromMD::exportIndividualDetCounts
 * @param datamdws
 * @param monitormdws
 * @param detid
 * @param vecX
 * @param vecY
 * @param xlabel
 * @param ylabel
 * @param donormalize
 */
void GetSpiceDataRawCountsFromMD::exportIndividualDetCounts(
    API::IMDEventWorkspace_const_sptr datamdws,
    API::IMDEventWorkspace_const_sptr monitormdws, const int detid,
    std::vector<double> &vecX, std::vector<double> &vecY, std::string &xlabel,
    std::string &ylabel, const bool &donormalize) {
  // Get detector counts
  std::vector<double> vecSampleLog;
  std::vector<double> vecDetCounts;
  int runnumber = -1;
  bool get2theta = false;
  if (xlabel.size() == 0) {
    // xlabel is in default and thus use 2-theta for X
    get2theta = true;
  }
  getDetCounts(datamdws, runnumber, detid, vecSampleLog, vecDetCounts,
               get2theta);
  if (!get2theta) {
    getSampleLogValues(datamdws, xlabel, runnumber, vecSampleLog);
  }
  if (vecSampleLog.size() != vecDetCounts.size())
    throw std::runtime_error("Logic error! number of sample logs should be "
                             "same as number of detector's counts.");

  // Get monitor counts
  std::vector<double> vec2thetaMon;
  std::vector<double> vecMonitorCounts;
  if (donormalize)
    getDetCounts(monitormdws, runnumber, detid, vec2thetaMon, vecMonitorCounts,
                 false);

  // FIXME - Consider refactoring in future
  // Normalize if required
  if (donormalize) {
    // check
    if (vecDetCounts.size() != vecMonitorCounts.size())
      throw std::runtime_error(
          "Number of detectors' counts' is different from that of "
          "monitor counts.");

    for (size_t i = 0; i < vecDetCounts.size(); ++i)
      if (vecMonitorCounts[i] > 1.0E-9)
        vecDetCounts[i] = vecDetCounts[i] / vecMonitorCounts[i];
      else
        vecDetCounts[i] = 0.;
  }

  // Sort and output
  const size_t numpts = vecDetCounts.size();
  std::vector<std::pair<double, double> > vecPair(numpts);
  for (size_t i = 0; i < numpts; ++i) {
    vecPair[i] = std::make_pair(vecSampleLog[i], vecDetCounts[i]);
  }
  std::sort(vecPair.begin(), vecPair.end());

  // Apply to output
  vecX.resize(numpts);
  vecY.resize(numpts);
  for (size_t i = 0; i < numpts; ++i) {
    vecX[i] = vecPair[i].first;
    vecY[i] = vecPair[i].second;
  }

  // Set up label
  if (get2theta)
    xlabel = "Degrees";
  if (donormalize)
    ylabel = "Noramlized Intensity";
  else
    ylabel = "Counts";
}

//----------------------------------------------------------------------------------------------
/** Export sample log values accross all runs
 * @brief GetSpiceDataRawCountsFromMD::exportSampleLogValue
 * @param datamdws
 * @param samplelogname :: name of the sample log to be exported
 * @param vecX :: output x values
 * @param vecY :: output y values
 * @param xlabel :: label of x-axis.  It is the name of another sample log.
 * @param ylabel
 */
void GetSpiceDataRawCountsFromMD::exportSampleLogValue(
    API::IMDEventWorkspace_const_sptr datamdws,
    const std::string &samplelogname, std::vector<double> &vecX,
    std::vector<double> &vecY, std::string &xlabel, std::string &ylabel) {
  // prepare
  vecX.clear();
  vecY.clear();

  // Y values
  int runnumber = -1;
  getSampleLogValues(datamdws, samplelogname, runnumber, vecY);
  ylabel = samplelogname;

  // X values
  if (xlabel.size() == 0) {
    // default
    xlabel = "Pt.";
  }
  getSampleLogValues(datamdws, xlabel, runnumber, vecX);

  if (vecX.size() != vecY.size())
    throw std::runtime_error(
        "It is not correct to have two different sizes vectors.");

  // Sort
  const size_t numpts = vecX.size();
  std::vector<std::pair<double, double> > vecPair(numpts);
  for (size_t i = 0; i < numpts; ++i) {
    vecPair[i] = std::make_pair(vecX[i], vecY[i]);
  }
  std::sort(vecPair.begin(), vecPair.end());

  // Apply to output
  vecX.resize(numpts);
  vecY.resize(numpts);
  for (size_t i = 0; i < numpts; ++i) {
    vecX[i] = vecPair[i].first;
    vecY[i] = vecPair[i].second;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Get detectors' counts
 * @brief GetSpiceDataRawCountsFromMD::getDetCounts
 * @param mdws
 * @param runnumber :: run number of the detectors having for exporting; -1 for
 * all run numbers
 * @param detid :: detector ID for the detectors for exporting; -1 for all
 * detectors
 * @param vecX :: x-values as 2theta position of detectors to be exported;
 * @param vecY :: raw detector's counts
 * @param formX :: flag to set up vecX
 */
void GetSpiceDataRawCountsFromMD::getDetCounts(
    API::IMDEventWorkspace_const_sptr mdws, const int &runnumber,
    const int &detid, std::vector<double> &vecX, std::vector<double> &vecY,
    bool formX) {
  // Get sample and source position
  if (mdws->getNumExperimentInfo() == 0)
    throw std::runtime_error(
        "There is no ExperimentInfo object that has been set to "
        "input MDEventWorkspace!");

  V3D samplepos;
  V3D sourcepos;

  if (formX) {
    ExperimentInfo_const_sptr expinfo = mdws->getExperimentInfo(0);
    Geometry::IComponent_const_sptr sample =
        expinfo->getInstrument()->getSample();
    samplepos = sample->getPos();
    g_log.debug() << "Sample position is " << samplepos.X() << ", "
                  << samplepos.Y() << ", " << samplepos.Z() << "\n";

    Geometry::IComponent_const_sptr source =
        expinfo->getInstrument()->getSource();
    sourcepos = source->getPos();
    g_log.debug() << "Source position is " << sourcepos.X() << ","
                  << sourcepos.Y() << ", " << sourcepos.Z() << "\n";
    vecX.clear();
  }
  vecY.clear();

  // Go through all events to find out their positions
  IMDIterator *mditer = mdws->createIterator();

  bool scancell = true;
  size_t nextindex = 1;
  while (scancell) {
    // get the number of events of this cell
    size_t numev2 = mditer->getNumEvents();
    g_log.debug() << "MDWorkspace " << mdws->name() << " Cell " << nextindex - 1
                  << ": Number of events = " << numev2
                  << " Does NEXT cell exist = " << mditer->next() << "\n";

    // loop over all the events in current cell
    for (size_t iev = 0; iev < numev2; ++iev) {
      // filter out the events with uninterrested run numbers and detid
      // runnumber/detid < 0 indicates that all run number or all detectors will
      // be taken
      int thisrunnumber = mditer->getInnerRunIndex(iev);
      if (runnumber >= 0 && thisrunnumber != runnumber)
        continue;

      int thisdetid = mditer->getInnerDetectorID(iev);
      if (detid >= 0 && thisdetid != detid)
        continue;

      // get detector position for 2theta
      if (formX) {
        double tempx = mditer->getInnerPosition(iev, 0);
        double tempy = mditer->getInnerPosition(iev, 1);
        double tempz = mditer->getInnerPosition(iev, 2);
        Kernel::V3D detpos(tempx, tempy, tempz);
        Kernel::V3D v_det_sample = detpos - samplepos;
        Kernel::V3D v_sample_src = samplepos - sourcepos;
        double twotheta = v_det_sample.angle(v_sample_src) / M_PI * 180.;
        vecX.push_back(twotheta);
      }

      // add new value to vecPair
      double signal = mditer->getInnerSignal(iev);
      vecY.push_back(signal);
    } // ENDFOR (iev)

    // Advance to next cell
    if (mditer->next()) {
      // advance to next cell
      mditer->jumpTo(nextindex);
      ++nextindex;
    } else {
      // break the loop
      scancell = false;
    }
  } // ENDOF(while)

  delete (mditer);

  return;
}

//----------------------------------------------------------------------------------------------
/** Get sample log values
 * @brief GetSpiceDataRawCountsFromMD::getSampleLogValues
 * @param mdws
 * @param samplelogname
 * @param runnumber
 * @param vecSampleLog
 */
void GetSpiceDataRawCountsFromMD::getSampleLogValues(
    IMDEventWorkspace_const_sptr mdws, const std::string &samplelogname,
    const int runnumber, std::vector<double> &vecSampleLog) {
  // Clear input
  vecSampleLog.clear();

  // Loop over all experiment info objects
  uint16_t numexpinfo = mdws->getNumExperimentInfo();
  for (uint16_t iexp = 0; iexp < numexpinfo; ++iexp) {
    // Get handler to experiment info object
    ExperimentInfo_const_sptr expinfo = mdws->getExperimentInfo(iexp);
    // Skip invalid run
    int thisrunnumber = expinfo->getRunNumber();
    if (thisrunnumber < 0)
      continue;
    // Skip unmatched run: input runnumber < 0 means that all run will be
    // accepted
    if (runnumber >= 0 && thisrunnumber != runnumber)
      continue;
    // Check property exists
    if (!expinfo->run().hasProperty(samplelogname)) {
      std::stringstream ess;
      ess << "Workspace " << mdws->name() << "'s " << iexp
          << "-th ExperimentInfo with "
             "run number " << thisrunnumber
          << " does not have specified property " << samplelogname;
      throw std::runtime_error(ess.str());
    }
    // Get experiment value
    double logvalue = expinfo->run().getPropertyAsSingleValue(samplelogname);
    vecSampleLog.push_back(logvalue);
    g_log.debug() << "Add sample log (" << samplelogname << ") " << logvalue
                  << " of " << iexp << "-th ExperimentInfo "
                  << "\n";
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Create output workspace
 * @brief GetSpiceDataRawCountsFromMD::createOutputWorkspace
 * @param vecX
 * @param vecY
 * @param xlabel :: only 'Degrees' can be applied to x-axis
 * @param ylabel
 * @return
 */
MatrixWorkspace_sptr GetSpiceDataRawCountsFromMD::createOutputWorkspace(
    const std::vector<double> &vecX, const std::vector<double> &vecY,
    const std::string &xlabel, const std::string &ylabel) {
  // Create MatrixWorkspace
  size_t sizex = vecX.size();
  size_t sizey = vecY.size();
  if (sizex != sizey || sizex == 0)
    throw std::runtime_error("Unable to create output matrix workspace.");

  MatrixWorkspace_sptr outws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey));
  if (!outws)
    throw std::runtime_error("Failed to create output matrix workspace.");

  // Set data
  MantidVec &dataX = outws->dataX(0);
  MantidVec &dataY = outws->dataY(0);
  MantidVec &dataE = outws->dataE(0);
  for (size_t i = 0; i < sizex; ++i) {
    dataX[i] = vecX[i];
    dataY[i] = vecY[i];
    if (dataY[i] > 1.)
      dataE[i] = sqrt(dataY[i]);
    else
      dataE[i] = 1.;
  }

  // Set label
  outws->setYUnitLabel(ylabel);
  if (xlabel.size() != 0) {
    try {
      outws->getAxis(0)->setUnit(xlabel);
    }
    catch (...) {
      g_log.information() << "Label " << xlabel << " for X-axis is not a unit "
                                                   "registered."
                          << "\n";
    }
  }

  return outws;
}

} // namespace MDAlgorithms
} // namespace Mantid
