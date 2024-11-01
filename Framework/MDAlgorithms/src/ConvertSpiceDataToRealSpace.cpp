// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventInserter.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <Poco/TemporaryFile.h>

namespace Mantid::MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

DECLARE_ALGORITHM(ConvertSpiceDataToRealSpace)

/** Init
 */
void ConvertSpiceDataToRealSpace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input table workspace for data.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("RunInfoWorkspace", "", Direction::Input),
                  "Input matrix workspace containing sample logs.  "
                  "It can be the RunInfoWorkspace output from LoadSpiceAscii. "
                  "It serves as parent workspace in the algorithm.");

  declareProperty("RunStart", "",
                  "User specified run start time of the experiment "
                  "in case that the run start time is not specified in the "
                  "input RunInfoWorkspace.");

  /// TODO - Add HB2B as it is implemented in future
  std::array<std::string, 1> allowedinstruments = {{"HB2A"}};
  auto instrumentvalidator = std::make_shared<ListValidator<std::string>>(allowedinstruments);
  declareProperty("Instrument", "HB2A", instrumentvalidator, "Instrument to be loaded. ");

  declareProperty("DetectorPrefix", "anode", "Prefix of the name for detectors. ");

  declareProperty("RunNumberName", "Pt.", "Log name for run number/measurement point.");

  declareProperty("RotationAngleLogName", "2theta", "Log name for rotation angle as the 2theta value of detector 0.");

  declareProperty("MonitorCountsLogName", "monitor", "Name of the sample log to record monitor counts of each run.");

  declareProperty("DurationLogName", "time", "Name of the sample log to record the duration of each run.");

  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name to use for the output workspace.");

  declareProperty(
      std::make_unique<WorkspaceProperty<IMDEventWorkspace>>("OutputMonitorWorkspace", "", Direction::Output),
      "Name to use for the output workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("DetectorEfficiencyTableWorkspace", "",
                                                                      Direction::Input, PropertyMode::Optional),
                  "Name of a table workspace containing the detectors' efficiency.");
}

/** Exec
 */
void ConvertSpiceDataToRealSpace::exec() {
  // Process inputs
  DataObjects::TableWorkspace_sptr dataTableWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr parentWS = getProperty("RunInfoWorkspace");
  m_instrumentName = getPropertyValue("Instrument");

  DataObjects::TableWorkspace_sptr detEffTableWS = getProperty("DetectorEfficiencyTableWorkspace");
  std::map<detid_t, double> detEffMap; // map for detector efficiency
  if (detEffTableWS)
    detEffMap = parseDetectorEfficiencyTable(detEffTableWS);

  // Check whether parent workspace has run start: order (1) parent ws, (2) user
  // given (3) nothing
  DateAndTime runstart(1000000000);
  bool hasrunstartset = false;
  if (parentWS->run().hasProperty("run_start")) {
    // Use parent workspace's first
    std::string runstartstr = parentWS->run().getProperty("run_start")->value();
    try {
      DateAndTime temprunstart(runstartstr);
      runstart = temprunstart;
      hasrunstartset = true;
    } catch (...) {
      g_log.warning() << "run_start from info matrix workspace is not correct. "
                      << "It cannot be convert from '" << runstartstr << "'."
                      << "\n";
    }
  }

  // from properties
  if (!hasrunstartset) {
    // Use user given
    std::string runstartstr = getProperty("RunStart");
    try {
      DateAndTime temprunstart(runstartstr);
      runstart = temprunstart;
      hasrunstartset = true;
    } catch (...) {
      g_log.warning() << "RunStart from input property is not correct. "
                      << "It cannot be convert from '" << runstartstr << "'."
                      << "\n";
    }
  }

  if (!hasrunstartset)
    g_log.warning("Run-start time is not defined either in "
                  "input parent workspace or given by user. 1990-01-01 "
                  "00:00:01 is used");

  // Convert table workspace to a list of 2D workspaces
  std::map<std::string, std::vector<double>> logvecmap;
  std::vector<Types::Core::DateAndTime> vectimes;

  // Set up range for x/y/z
  m_extentMins.resize(3);
  m_extentMaxs.resize(3);
  for (size_t i = 0; i < 3; ++i) {
    m_extentMins[i] = DBL_MAX;
    m_extentMaxs[i] = -DBL_MAX;
  }

  std::vector<MatrixWorkspace_sptr> vec_ws2d =
      convertToMatrixWorkspace(dataTableWS, parentWS, runstart, logvecmap, vectimes);

  // Apply detector efficiency
  if (!detEffMap.empty())
    correctByDetectorEfficiency(vec_ws2d, detEffMap);

  // check range for x/y/z
  m_numBins.resize(3);
  for (size_t d = 0; d < 3; ++d) {
    if (fabs(m_extentMins[d] - m_extentMaxs[d]) < 1.0E-6) {
      // Range is too small so treat it as 1 value
      double mvalue = m_extentMins[d];
      m_extentMins[d] = mvalue - 0.1;
      m_extentMaxs[d] = mvalue + 0.1;
      m_numBins[d] = 1;
    } else {
      m_numBins[d] = 100;
    }
  }

  // Convert to MD workspaces
  g_log.debug("About to converting to workspaces done!");
  IMDEventWorkspace_sptr m_mdEventWS = createDataMDWorkspace(vec_ws2d);
  std::string monitorlogname = getProperty("MonitorCountsLogName");
  IMDEventWorkspace_sptr mdMonitorWS = createMonitorMDWorkspace(vec_ws2d, logvecmap[monitorlogname]);

  // Add experiment info for each run and sample log to the first experiment
  // info object
  addExperimentInfos(m_mdEventWS, vec_ws2d);
  addExperimentInfos(mdMonitorWS, vec_ws2d);
  appendSampleLogs(m_mdEventWS, logvecmap, vectimes);

  // Set property
  setProperty("OutputWorkspace", m_mdEventWS);
  setProperty("OutputMonitorWorkspace", mdMonitorWS);
}

//------------------------------------------------------------------------------------------------
/** Convert runs/pts from table workspace to a list of workspace 2D
 * @brief ConvertSpiceDataToRealSpace::convertToWorkspaces
 * @param tablews
 * @param parentws
 * @param runstart
 * @param logvecmap
 * @param vectimes
 * @return
 */
std::vector<MatrixWorkspace_sptr> ConvertSpiceDataToRealSpace::convertToMatrixWorkspace(
    const DataObjects::TableWorkspace_sptr &tablews, const API::MatrixWorkspace_const_sptr &parentws,
    Types::Core::DateAndTime runstart, std::map<std::string, std::vector<double>> &logvecmap,
    std::vector<Types::Core::DateAndTime> &vectimes) {
  // Get table workspace's column information
  size_t ipt, irotangle, itime;
  std::vector<std::pair<size_t, size_t>> anodelist;
  std::map<std::string, size_t> sampleindexlist;
  readTableInfo(tablews, ipt, irotangle, itime, anodelist, sampleindexlist);
  m_numSpec = anodelist.size();

  // Load data
  size_t numws = tablews->rowCount();
  std::vector<MatrixWorkspace_sptr> vecws(numws);
  double duration = 0;
  vectimes.resize(numws);
  for (size_t irow = 0; irow < numws; ++irow) {
    vecws[irow] = loadRunToMatrixWS(tablews, irow, parentws, runstart, ipt, irotangle, itime, anodelist, duration);
    vectimes[irow] = runstart;
    runstart += static_cast<int64_t>(duration * 1.0E9);
  }

  // Process log data which will not be put to matrix workspace but will got to
  // MDWorkspace
  parseSampleLogs(tablews, sampleindexlist, logvecmap);

  g_log.debug() << "Number of matrix workspaces in vector = " << vecws.size() << "\n";
  return vecws;
}

//------------------------------------------------------------------------------------------------
/** Parse sample logs from table workspace and return with a set of vectors
 * @brief ConvertSpiceDataToRealSpace::parseSampleLogs
 * @param tablews
 * @param indexlist
 * @param logvecmap
 */
void ConvertSpiceDataToRealSpace::parseSampleLogs(const DataObjects::TableWorkspace_sptr &tablews,
                                                  const std::map<std::string, size_t> &indexlist,
                                                  std::map<std::string, std::vector<double>> &logvecmap) {
  size_t numrows = tablews->rowCount();

  std::map<std::string, size_t>::const_iterator indexiter;
  for (indexiter = indexlist.begin(); indexiter != indexlist.end(); ++indexiter) {
    std::string logname = indexiter->first;
    size_t icol = indexiter->second;

    g_log.debug() << " Parsing log " << logname << "\n";

    std::vector<double> logvec(numrows);
    for (size_t ir = 0; ir < numrows; ++ir) {
      auto dbltemp = tablews->cell_cast<double>(ir, icol);
      logvec[ir] = dbltemp;
    }

    logvecmap.emplace(logname, logvec);
  }
}

//------------------------------------------------------------------------------------------------
/** Load one run of data to a new workspace
 * @brief ConvertSpiceDataToRealSpace::loadRunToMatrixWS
 * @param tablews
 * @param irow
 * @param parentws
 * @param runstart
 * @param ipt
 * @param irotangle
 * @param itime
 * @param anodelist
 * @param duration
 * @return
 */
MatrixWorkspace_sptr ConvertSpiceDataToRealSpace::loadRunToMatrixWS(
    const DataObjects::TableWorkspace_sptr &tablews, size_t irow, const MatrixWorkspace_const_sptr &parentws,
    Types::Core::DateAndTime runstart, size_t ipt, size_t irotangle, size_t itime,
    const std::vector<std::pair<size_t, size_t>> &anodelist, double &duration) {
  // New workspace from parent workspace
  MatrixWorkspace_sptr tempws = WorkspaceFactory::Instance().create(parentws, m_numSpec, 2, 1);

  // Set up angle, time and run number
  double twotheta = tablews->cell<double>(irow, irotangle);
  TimeSeriesProperty<double> *prop2theta = new TimeSeriesProperty<double>("rotangle");

  prop2theta->addValue(runstart, twotheta);
  tempws->mutableRun().addProperty(prop2theta);

  TimeSeriesProperty<std::string> *proprunstart = new TimeSeriesProperty<std::string>("run_start");
  proprunstart->addValue(runstart, runstart.toISO8601String());

  g_log.debug() << "Run " << irow << ": set run start to " << runstart.toISO8601String() << "\n";
  if (tempws->run().hasProperty("run_start")) {
    g_log.information() << "Temporary workspace inherites run_start as "
                        << tempws->run().getProperty("run_start")->value()
                        << ". It will be replaced by the correct value. "
                        << "\n";
    tempws->mutableRun().removeProperty("run_start");
  }
  tempws->mutableRun().addProperty(proprunstart);

  int pt = tablews->cell<int>(irow, ipt);
  tempws->mutableRun().addProperty(new PropertyWithValue<int>("run_number", pt));

  // Load instrument
  auto instloader = createChildAlgorithm("LoadInstrument");
  instloader->initialize();
  instloader->setProperty("InstrumentName", m_instrumentName);
  instloader->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  instloader->setProperty("Workspace", tempws);
  instloader->execute();

  tempws = instloader->getProperty("Workspace");

  // Import data
  const auto &specInfo = tempws->spectrumInfo();
  for (size_t i = 0; i < m_numSpec; ++i) {
    const auto &pos = specInfo.position(i);
    tempws->mutableX(i)[0] = pos[0] + 0.01;
    double yvalue = tablews->cell<double>(irow, anodelist[i].second);
    tempws->mutableY(i)[0] = yvalue;
    tempws->mutableE(i)[0] = std::max(sqrt(yvalue), 1.0);
    // update X-range, Y-range and Z-range
    for (size_t d = 0; d < 3; ++d) {
      if (pos[d] < m_extentMins[d])
        m_extentMins[d] = pos[d];
      if (pos[d] > m_extentMaxs[d])
        m_extentMaxs[d] = pos[d];
    }
  }

  // Return duration
  duration = tablews->cell<double>(irow, itime);

  return tempws;
}

//------------------------------------------------------------------------------------------------
/** Read table workspace's column information
 * @brief ConvertSpiceDataToRealSpace::readTableInfo
 * @param tablews
 * @param ipt
 * @param irotangle
 * @param itime
 * @param anodelist
 * @param samplenameindexmap
 */
void ConvertSpiceDataToRealSpace::readTableInfo(const TableWorkspace_const_sptr &tablews, size_t &ipt,
                                                size_t &irotangle, size_t &itime,
                                                std::vector<std::pair<size_t, size_t>> &anodelist,
                                                std::map<std::string, size_t> &samplenameindexmap) {

  // Get detectors' names and other sample names
  std::string anodelogprefix = getProperty("DetectorPrefix");
  const std::vector<std::string> &colnames = tablews->getColumnNames();
  for (size_t icol = 0; icol < colnames.size(); ++icol) {
    const std::string &colname = colnames[icol];

    if (colname.starts_with(anodelogprefix)) {
      // anode
      std::vector<std::string> terms;
      boost::split(terms, colname, boost::is_any_of(anodelogprefix));
      auto anodeid = static_cast<size_t>(std::stoi(terms.back()));
      anodelist.emplace_back(anodeid, icol);
    } else {
      samplenameindexmap.emplace(colname, icol);
    }
  } // ENDFOR (icol)

  // Check detectors' names
  if (anodelist.empty()) {
    std::stringstream errss;
    errss << "There is no log name starting with " << anodelogprefix << " for detector. ";
    throw std::runtime_error(errss.str());
  }

  // Find out other essential sample log names
  std::map<std::string, size_t>::iterator mapiter;

  std::string ptname = getProperty("RunNumberName");                 // "Pt."
  std::string monitorlogname = getProperty("MonitorCountsLogName");  //"monitor"
  std::string durationlogname = getProperty("DurationLogName");      //"time"
  std::string rotanglelogname = getProperty("RotationAngleLogName"); // "2theta"

  std::vector<std::string> lognames{ptname, monitorlogname, durationlogname, rotanglelogname};

  std::vector<size_t> ilognames(lognames.size());

  for (size_t i = 0; i < lognames.size(); ++i) {
    const std::string &logname = lognames[i];
    mapiter = samplenameindexmap.find(logname);
    if (mapiter != samplenameindexmap.end()) {
      ilognames[i] = mapiter->second;
    } else {
      std::stringstream ess;
      ess << "Essential log name " << logname << " cannot be found in data table workspace.";
      throw std::runtime_error(ess.str());
    }
  }

  // Retrieve the vector index
  ipt = ilognames[0];
  itime = ilognames[2];
  irotangle = ilognames[3];

  // Sort out anode id index list;
  std::sort(anodelist.begin(), anodelist.end());
}

//------------------------------------------------------------------------------------------------
/** Create sample logs for MD workspace
 * @brief LoadHFIRPDD::appendSampleLogs
 * @param mdws
 * @param logvecmap
 * @param vectimes
 */
void ConvertSpiceDataToRealSpace::appendSampleLogs(const IMDEventWorkspace_sptr &mdws,
                                                   const std::map<std::string, std::vector<double>> &logvecmap,
                                                   const std::vector<Types::Core::DateAndTime> &vectimes) {
  // Check!
  size_t numexpinfo = mdws->getNumExperimentInfo();
  if (numexpinfo == 0)
    throw std::runtime_error("There is no ExperimentInfo defined for MDWorkspace. "
                             "It is impossible to add any log!");
  else if (numexpinfo != vectimes.size() + 1)
    throw std::runtime_error("The number of ExperimentInfo should be 1 more than "
                             "the length of vector of time, i.e., number of matrix workspaces.");

  std::map<std::string, std::vector<double>>::const_iterator miter;

  // get runnumber vector
  std::string runnumlogname = getProperty("RunNumberName");
  miter = logvecmap.find(runnumlogname);
  if (miter == logvecmap.end())
    throw std::runtime_error("Impossible not to find Pt. in log vec map.");
  const std::vector<double> &vecrunno = miter->second;

  // Add run_start and start_time to each ExperimentInfo
  for (size_t i = 0; i < vectimes.size(); ++i) {
    Types::Core::DateAndTime runstart = vectimes[i];
    mdws->getExperimentInfo(static_cast<uint16_t>(i))
        ->mutableRun()
        .addLogData(new PropertyWithValue<std::string>("run_start", runstart.toFormattedString()));
  }
  mdws->getExperimentInfo(static_cast<uint16_t>(vectimes.size()))
      ->mutableRun()
      .addLogData(new PropertyWithValue<std::string>("run_start", vectimes[0].toFormattedString()));

  // Add sample logs
  // get hold of last experiment info
  ExperimentInfo_sptr eilast = mdws->getExperimentInfo(static_cast<uint16_t>(numexpinfo - 1));

  for (miter = logvecmap.begin(); miter != logvecmap.end(); ++miter) {
    std::string logname = miter->first;
    const std::vector<double> &veclogval = miter->second;

    // Check log values and times
    if (veclogval.size() != vectimes.size()) {
      g_log.error() << "Log " << logname << " has different number of log values (" << veclogval.size()
                    << ") than number of log entry time (" << vectimes.size() << ")"
                    << "\n";
      continue;
    }

    // For N single value experiment info
    for (uint16_t i = 0; i < static_cast<uint16_t>(veclogval.size()); ++i) {
      // get ExperimentInfo
      ExperimentInfo_sptr tmpei = mdws->getExperimentInfo(i);
      // check run number matches
      int runnumber = std::stoi(tmpei->run().getProperty("run_number")->value());
      if (runnumber != static_cast<int>(vecrunno[i]))
        throw std::runtime_error("Run number does not match to Pt. value.");
      // add property
      tmpei->mutableRun().addLogData(new PropertyWithValue<double>(logname, veclogval[i]));
    }

    // Create a new log
    auto templog = new TimeSeriesProperty<double>(logname);
    templog->addValues(vectimes, veclogval);

    // Add log to experiment info
    eilast->mutableRun().addLogData(templog);
  }
}

//------------------------------------------------------------------------------------------------
/** Add Experiment Info to the MDWorkspace.  Add 1+N ExperimentInfo
 * @brief ConvertSpiceDataToRealSpace::addExperimentInfos
 * @param mdws
 * @param vec_ws2d
 */
void ConvertSpiceDataToRealSpace::addExperimentInfos(const API::IMDEventWorkspace_sptr &mdws,
                                                     const std::vector<API::MatrixWorkspace_sptr> &vec_ws2d) {
  // Add N experiment info as there are N measurment points
  for (const auto &ws2d : vec_ws2d) {
    // Create an ExperimentInfo object
    ExperimentInfo_sptr tmp_expinfo = std::make_shared<ExperimentInfo>();
    Geometry::Instrument_const_sptr tmp_inst = ws2d->getInstrument();
    tmp_expinfo->setInstrument(tmp_inst);

    int runnumber = std::stoi(ws2d->run().getProperty("run_number")->value());
    tmp_expinfo->mutableRun().addProperty(new PropertyWithValue<int>("run_number", runnumber));

    // Add ExperimentInfo to workspace
    mdws->addExperimentInfo(tmp_expinfo);
  }

  // Add one additional in order to contain the combined sample logs
  ExperimentInfo_sptr combine_expinfo = std::make_shared<ExperimentInfo>();
  combine_expinfo->mutableRun().addProperty(new PropertyWithValue<int>("run_number", -1));
  mdws->addExperimentInfo(combine_expinfo);
}

//------------------------------------------------------------------------------------------------
/** Convert to MD Event workspace
 * @brief ConvertSpiceDataToRealSpace::convertToMDEventWS
 * @param vec_ws2d
 * @return
 */
IMDEventWorkspace_sptr
ConvertSpiceDataToRealSpace::createDataMDWorkspace(const std::vector<MatrixWorkspace_sptr> &vec_ws2d) {

  // Create a target output workspace.
  IMDEventWorkspace_sptr outWs = MDEventFactory::CreateMDWorkspace(m_nDimensions, "MDEvent");

  // Extract Dimensions and add to the output workspace.

  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "x";
  vec_ID[1] = "y";
  vec_ID[2] = "z";

  std::vector<std::string> vec_name(3);
  vec_name[0] = "X";
  vec_name[1] = "Y";
  vec_name[2] = "Z";

  // Create MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame frame(Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");

  // Add dimensions
  for (size_t i = 0; i < m_nDimensions; ++i) {
    std::string id = vec_ID[i];
    std::string name = vec_name[i];

    for (size_t d = 0; d < 3; ++d)
      g_log.debug() << "Direction " << d << ", Range = " << m_extentMins[d] << ", " << m_extentMaxs[d] << "\n";
    outWs->addDimension(Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
        id, name, frame, static_cast<coord_t>(m_extentMins[i]), static_cast<coord_t>(m_extentMaxs[i]), m_numBins[i])));
  }

  // Add events
  // Creates a new instance of the MDEventInserter.
  MDEventWorkspace<MDEvent<3>, 3>::sptr MDEW_MDEVENT_3 =
      std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(outWs);

  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(MDEW_MDEVENT_3);

  for (const auto &thisWorkspace : vec_ws2d) {
    uint16_t runnumber = static_cast<uint16_t>(std::stoi(thisWorkspace->run().getProperty("run_number")->value()));

    detid_t detindex = 0;

    size_t nHist = thisWorkspace->getNumberHistograms();
    const auto &specInfo = thisWorkspace->spectrumInfo();
    for (std::size_t i = 0; i < nHist; ++i) {
      const auto &vecsignal = thisWorkspace->y(i);
      const auto &vecerror = thisWorkspace->e(i);
      auto signal = static_cast<float>(vecsignal[0]);
      auto error = static_cast<float>(vecerror[0]);
      detid_t detid = specInfo.detector(i).getID() + detindex;
      const auto &detPos = specInfo.position(i);
      Mantid::coord_t data[3];
      data[0] = static_cast<float>(detPos.X());
      data[1] = static_cast<float>(detPos.Y());
      data[2] = static_cast<float>(detPos.Z());
      inserter.insertMDEvent(signal, error * error, runnumber, 0, detid, data);
    } // ENDFOR(spectrum)
  } // ENDFOR (workspace)

  return outWs;
}

//------------------------------------------------------------------------------------------------
/** Create an MDWorkspace for monitoring counts.
 * @brief LoadHFIRPDD::createMonitorMDWorkspace
 * @param vec_ws2d
 * @param vecmonitor
 * @return
 */
IMDEventWorkspace_sptr
ConvertSpiceDataToRealSpace::createMonitorMDWorkspace(const std::vector<MatrixWorkspace_sptr> &vec_ws2d,
                                                      const std::vector<double> &vecmonitor) {
  // Create a target output workspace.
  IMDEventWorkspace_sptr outWs = MDEventFactory::CreateMDWorkspace(m_nDimensions, "MDEvent");

  // Extract Dimensions and add to the output workspace.

  std::vector<std::string> vec_ID(3);
  vec_ID[0] = "x";
  vec_ID[1] = "y";
  vec_ID[2] = "z";

  std::vector<std::string> vec_name(3);
  vec_name[0] = "X";
  vec_name[1] = "Y";
  vec_name[2] = "Z";

  // Create MDFrame of General Frame type
  Mantid::Geometry::GeneralFrame frame(Mantid::Geometry::GeneralFrame::GeneralFrameDistance, "m");

  // Add dimensions
  for (size_t i = 0; i < m_nDimensions; ++i) {
    std::string id = vec_ID[i];
    std::string name = vec_name[i];

    outWs->addDimension(Geometry::MDHistoDimension_sptr(new Geometry::MDHistoDimension(
        id, name, frame, static_cast<coord_t>(m_extentMins[i]), static_cast<coord_t>(m_extentMaxs[i]), m_numBins[i])));
  }

  // Add events
  // Creates a new instance of the MDEventInserter.
  MDEventWorkspace<MDEvent<3>, 3>::sptr MDEW_MDEVENT_3 =
      std::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3>>(outWs);

  MDEventInserter<MDEventWorkspace<MDEvent<3>, 3>::sptr> inserter(MDEW_MDEVENT_3);

  for (size_t iws = 0; iws < vec_ws2d.size(); ++iws) {
    API::MatrixWorkspace_sptr thisWorkspace = vec_ws2d[iws];
    short unsigned int runnumber =
        static_cast<short unsigned int>(std::stoi(thisWorkspace->run().getProperty("run_number")->value()));

    detid_t detindex = 0;
    auto signal = static_cast<float>(vecmonitor[iws]);
    float error = 1;
    if (signal > 1)
      error = std::sqrt(signal);

    size_t nHist = thisWorkspace->getNumberHistograms();
    const auto &specInfo = thisWorkspace->spectrumInfo();
    for (std::size_t i = 0; i < nHist; ++i) {
      // For each spectrum/detector
      detid_t detid = specInfo.detector(i).getID() + detindex;
      const auto &detPos = specInfo.position(i);
      Mantid::coord_t data[3];
      data[0] = static_cast<float>(detPos.X());
      data[1] = static_cast<float>(detPos.Y());
      data[2] = static_cast<float>(detPos.Z());
      inserter.insertMDEvent(signal, error * error, runnumber, 0, detid, data);
    } // ENDFOR(spectrum)
  } // ENDFOR (workspace)

  return outWs;
}

//------------------------------------------------------------------------------------------------
/** Parse detector efficiency from table workspace to map
 * @brief ConvertSpiceDataToRealSpace::parseDetectorEfficiencyTable
 * @param detefftablews :: [input] detector efficiency table workspace
 * @returns detector efficiency map
 */
std::map<detid_t, double>
ConvertSpiceDataToRealSpace::parseDetectorEfficiencyTable(const DataObjects::TableWorkspace_sptr &detefftablews) {
  std::map<detid_t, double> deteffmap;

  // check table workspace
  size_t numcols = detefftablews->columnCount();
  if (numcols != 2)
    throw std::runtime_error("Input tableworkspace must have 2 and only 2 columns.");

  // parse the detector
  size_t numrows = detefftablews->rowCount();
  for (size_t i = 0; i < numrows; ++i) {
    detid_t detid = detefftablews->cell<detid_t>(i, 0);
    double deteff = detefftablews->cell<double>(i, 1);
    deteffmap.emplace(detid, deteff);
  }

  return deteffmap;
}

//------------------------------------------------------------------------------------------------
/** Apply the detector's efficiency correction to
 * @brief ConvertSpiceDataToRealSpace::correctByDetectorEfficiency
 * @param vec_ws2d
 * @param detEffMap
 */
void ConvertSpiceDataToRealSpace::correctByDetectorEfficiency(std::vector<MatrixWorkspace_sptr> vec_ws2d,
                                                              const std::map<detid_t, double> &detEffMap) {
  std::vector<MatrixWorkspace_sptr>::iterator it;
  std::map<detid_t, double>::const_iterator detiter;
  for (it = vec_ws2d.begin(); it != vec_ws2d.end(); ++it) {
    MatrixWorkspace_sptr ws = *it;
    const auto &specInfo = ws->spectrumInfo();
    size_t numspec = ws->getNumberHistograms();
    for (size_t iws = 0; iws < numspec; ++iws) {
      detid_t detid = specInfo.detector(iws).getID();
      detiter = detEffMap.find(detid);
      if (detiter != detEffMap.end())
        ws->mutableY(iws)[0] /= detiter->second;
    }
  }
}

} // namespace Mantid::MDAlgorithms
