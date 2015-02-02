#include "MantidMDAlgorithms/LoadHFIRPDData.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string/predicate.hpp>
#include <Poco/TemporaryFile.h>

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

DECLARE_ALGORITHM(LoadHFIRPDData)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadHFIRPDData::LoadHFIRPDData() : m_instrumentName(""), m_numSpec(0) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadHFIRPDData::~LoadHFIRPDData() {}

//----------------------------------------------------------------------------------------------
/** Init
 */
void LoadHFIRPDData::init() {
  declareProperty(new WorkspaceProperty<TableWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "Input table workspace for data.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("ParentWorkspace", "",
                                                         Direction::Input),
                  "Input matrix workspace serving as parent workspace "
                  "containing sample logs.");

  declareProperty("RunStart", "", "Run start time");

  declareProperty("Instrument", "HB2A", "Instrument to be loaded. ");

  declareProperty("InitRunNumber", 1, "Starting value for run number.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name to use for the output workspace.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputMonitorWorkspace", "", Direction::Output),
                  "Name to use for the output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Exec
 */
void LoadHFIRPDData::exec() {
  /** Stage 2
  // Process inputs
  std::string spiceFileName = getProperty("Filename");

  // Load SPICE data
  m_dataTableWS = loadSpiceData(spiceFileName);
  */
  m_dataTableWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr parentWS = getProperty("ParentWorkspace");

  m_instrumentName = getPropertyValue("Instrument");

  // Check whether parent workspace has run start
  DateAndTime runstart(0);
  if (parentWS->run().hasProperty("run_start")) {
    // Use parent workspace's first
    runstart = parentWS->run().getProperty("run_start")->value();
  } else {
    // Use user given
    std::string runstartstr = getProperty("RunStart");
    // raise exception if user does not give a proper run start
    if (runstartstr.size() == 0)
      throw std::runtime_error("Run-start time is not defined either in "
                               "input parent workspace or given by user.");
    runstart = DateAndTime(runstartstr);
  }

  // Convert table workspace to a list of 2D workspaces
  std::map<std::string, std::vector<double> > logvecmap;
  std::vector<Kernel::DateAndTime> vectimes;
  std::vector<MatrixWorkspace_sptr> vec_ws2d = convertToWorkspaces(
      m_dataTableWS, parentWS, runstart, logvecmap, vectimes);

  g_log.notice() << "[DB] Convert to workspaces done!"
                 << "\n";

  // Convert to MD workspaces
  IMDEventWorkspace_sptr m_mdEventWS = convertToMDEventWS(vec_ws2d);
  IMDEventWorkspace_sptr mdMonitorWS =
      createMonitorMDWorkspace(vec_ws2d, logvecmap);

  // Add experiment info for each run and sample log to the first experiment
  // info object
  int initrunnumber = getProperty("InitRunNumber");
  addExperimentInfos(m_mdEventWS, vec_ws2d, initrunnumber);
  addExperimentInfos(mdMonitorWS, vec_ws2d, initrunnumber);
  appendSampleLogs(m_mdEventWS, logvecmap, vectimes);

  // Set property
  g_log.notice() << "[DB] Check point!"
                 << "\n";
  setProperty("OutputWorkspace", m_mdEventWS);
  setProperty("OutputMonitorWorkspace", mdMonitorWS);
}

//----------------------------------------------------------------------------------------------
/** Load data by call
 */
TableWorkspace_sptr
LoadHFIRPDData::loadSpiceData(const std::string &spicefilename) {
  const std::string tempoutws = "_tempoutdatatablews";
  const std::string tempinfows = "_tempinfomatrixws";

  IAlgorithm_sptr loader =
      this->createChildAlgorithm("LoadSPICEAscii", 0, 5, true);

  loader->initialize();
  loader->setProperty("Filename", spicefilename);
  loader->setPropertyValue("OutputWorkspace", tempoutws);
  loader->setPropertyValue("RunInfoWorkspace", tempinfows);
  loader->executeAsChildAlg();

  TableWorkspace_sptr tempdatatablews = loader->getProperty("OutputWorkspace");
  if (tempdatatablews)
    g_log.notice() << "[DB] data table contains " << tempdatatablews->rowCount()
                   << " lines."
                   << "\n";
  else
    g_log.notice("No table workspace is returned.");

  return tempdatatablews;
}

//----------------------------------------------------------------------------------------------
/** Convert runs/pts from table workspace to a list of workspace 2D
 */
std::vector<MatrixWorkspace_sptr> LoadHFIRPDData::convertToWorkspaces(
    DataObjects::TableWorkspace_sptr tablews,
    API::MatrixWorkspace_const_sptr parentws, Kernel::DateAndTime runstart,
    std::map<std::string, std::vector<double> > &logvecmap,
    std::vector<Kernel::DateAndTime> &vectimes) {
  // Get table workspace's column information
  size_t ipt, irotangle, itime;
  std::vector<std::pair<size_t, size_t> > anodelist;
  std::map<std::string, size_t> sampleindexlist;
  readTableInfo(tablews, ipt, irotangle, itime, anodelist, sampleindexlist);
  g_log.notice() << "[DB] Check point 1: Number of anodelist = "
                 << anodelist.size() << "\n";
  m_numSpec = anodelist.size();

  // Load data
  size_t numws = tablews->rowCount();
  std::vector<MatrixWorkspace_sptr> vecws(numws);
  double duration = 0;
  vectimes.resize(numws);
  for (size_t i = 0; i < numws; ++i) {
    vecws[i] = loadRunToMatrixWS(tablews, i, parentws, runstart, irotangle,
                                 itime, anodelist, duration);
    vectimes[i] = runstart;
    runstart += static_cast<int64_t>(duration * 1.0E9);
  }

  // Process log data which will not be put to matrix workspace but will got to
  // MDWorkspace
  parseSampleLogs(tablews, sampleindexlist, ipt, logvecmap);

  g_log.notice() << "[DB] Number of matrix workspaces in vector = "
                 << vecws.size() << "\n";
  return vecws;
}

//------------------------------------------------------------------------------------------------
/**
 * @brief LoadHFIRPDD::parseSampleLogs
 * @param tablews
 * @param indexlist
 * @param ipt :: index for Pt. which will be not be written
 * @param logvecmap
 */
void LoadHFIRPDData::parseSampleLogs(
    DataObjects::TableWorkspace_sptr tablews,
    const std::map<std::string, size_t> &indexlist, size_t ipt,
    std::map<std::string, std::vector<double> > &logvecmap) {
  size_t numrows = tablews->rowCount();

  std::map<std::string, size_t>::const_iterator indexiter;
  for (indexiter = indexlist.begin(); indexiter != indexlist.end();
       ++indexiter) {
    std::string logname = indexiter->first;
    size_t icol = indexiter->second;

    g_log.notice() << "[DB] "
                   << " About to parse log " << logname << "\n";

    std::vector<double> logvec(numrows);
    for (size_t ir = 0; ir < numrows; ++ir) {
      double dbltemp = tablews->cell_cast<double>(ir, icol);
      logvec[ir] = dbltemp;
    }

    logvecmap.insert(std::make_pair(logname, logvec));
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Load one run of data to a new workspace
 * @brief LoadHFIRPDD::loadRunToMatrixWS
 * @param tablews :: input workspace
 * @param irow :: the row in workspace to load
 * @param parentws :: parent workspace with preset log
 * @param runstart :: run star time
 * @param irotangle :: column index of rotation angle
 * @param itime :: column index of duration
 * @param anodelist :: list of anodes
 * @param duration :: output of duration
 * @return
 */
MatrixWorkspace_sptr LoadHFIRPDData::loadRunToMatrixWS(
    DataObjects::TableWorkspace_sptr tablews, size_t irow,
    MatrixWorkspace_const_sptr parentws, Kernel::DateAndTime runstart,
    size_t irotangle, size_t itime,
    const std::vector<std::pair<size_t, size_t> > anodelist, double &duration) {
  g_log.notice() << "[DB] m_numSpec = " << m_numSpec
                 << ", Instrument name = " << m_instrumentName << ". \n";
  // New workspace from parent workspace
  MatrixWorkspace_sptr tempws =
      WorkspaceFactory::Instance().create(parentws, m_numSpec, 2, 1);

  // Set up angle and time
  double twotheta = tablews->cell<double>(irow, irotangle);
  TimeSeriesProperty<double> *prop2theta =
      new TimeSeriesProperty<double>("rotangle");

  prop2theta->addValue(runstart, twotheta);
  tempws->mutableRun().addProperty(prop2theta);

  TimeSeriesProperty<std::string> *proprunstart =
      new TimeSeriesProperty<std::string>("run_start");
  proprunstart->addValue(runstart, runstart.toISO8601String());

  g_log.notice() << "[DB] Trying to set run start to "
                 << runstart.toISO8601String() << "\n";
  if (tempws->run().hasProperty("run_start")) {
    g_log.error() << "Temp workspace exists run_start as "
                  << tempws->run().getProperty("run_start")->value() << "\n";
    tempws->mutableRun().removeProperty("run_start");
  }
  tempws->mutableRun().addProperty(proprunstart);

  // Load instrument
  IAlgorithm_sptr instloader = this->createChildAlgorithm("LoadInstrument");
  instloader->initialize();
  instloader->setProperty("InstrumentName", m_instrumentName);
  instloader->setProperty("Workspace", tempws);
  instloader->execute();

  tempws = instloader->getProperty("Workspace");

  // Import data
  for (size_t i = 0; i < m_numSpec; ++i) {
    Geometry::IDetector_const_sptr tmpdet = tempws->getDetector(i);
    tempws->dataX(i)[0] = tmpdet->getPos().X();
    tempws->dataX(i)[0] = tmpdet->getPos().X() + 0.01;
    tempws->dataY(i)[0] = tablews->cell<double>(irow, anodelist[i].second);
    tempws->dataE(i)[0] = 1;
  }

  // Return duration
  duration = tablews->cell<double>(irow, itime);

  return tempws;
}

//----------------------------------------------------------------------------------------------
/** Read table workspace's column information
 * @brief LoadHFIRPDD::readTableInfo
 * @param tablews
 * @param ipt
 * @param irotangle
 * @param itime
 * @param anodelist
 */
void
LoadHFIRPDData::readTableInfo(TableWorkspace_const_sptr tablews, size_t &ipt,
                           size_t &irotangle, size_t &itime,
                           std::vector<std::pair<size_t, size_t> > &anodelist,
                           std::map<std::string, size_t> &sampleindexlist) {
  // Init
  bool bfPt = false;
  bool bfRotAngle = false;
  bool bfTime = false;

  const std::vector<std::string> &colnames = tablews->getColumnNames();

  for (size_t icol = 0; icol < colnames.size(); ++icol) {
    const std::string &colname = colnames[icol];

    if (boost::starts_with(colname, "anode")) {
      // anode
      std::vector<std::string> terms;
      boost::split(terms, colname, boost::is_any_of("anode"));
      size_t anodeid = static_cast<size_t>(atoi(terms.back().c_str()));
      anodelist.push_back(std::make_pair(anodeid, icol));
    } else {
      sampleindexlist.insert(std::make_pair(colname, icol));
    }
  }

  // Find out
  std::map<std::string, size_t>::iterator mapiter;

  // Pt.
  mapiter = sampleindexlist.find("Pt.");
  if (mapiter != sampleindexlist.end()) {
    ipt = mapiter->second;
    bfPt = true;
  }

  // 2theta_zero
  mapiter = sampleindexlist.find("2theta");
  if (mapiter != sampleindexlist.end()) {
    irotangle = mapiter->second;
    bfRotAngle = true;
  }

  // time
  mapiter = sampleindexlist.find("time");
  if (mapiter != sampleindexlist.end()) {
    itime = mapiter->second;
    bfTime = true;
  }

  if (!(bfTime && bfPt && bfRotAngle)) {
    throw std::runtime_error(
        "At least 1 of these 3 is not found: Pt., 2theta, time");
  }

  // Sort out anode id index list;
  std::sort(anodelist.begin(), anodelist.end());

  return;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief LoadHFIRPDD::createParentWorkspace
 * @param numspec
 * @return
 */
API::MatrixWorkspace_sptr LoadHFIRPDData::createParentWorkspace(size_t numspec) {
  // TODO - This method might be deleted

  MatrixWorkspace_sptr tempws =
      WorkspaceFactory::Instance().create("Workspace2D", numspec, 2, 1);

  // FIXME - Need unit

  // TODO - Load property from

  return tempws;
}

//----------------------------------------------------------------------------------------------
/** Convert to MD Event workspace
 */
IMDEventWorkspace_sptr LoadHFIRPDData::convertToMDEventWS(
    const std::vector<MatrixWorkspace_sptr> vec_ws2d) {
  // Write the lsit of workspacs to a file to be loaded to an MD workspace
  Poco::TemporaryFile tmpFile;
  std::string tempFileName = tmpFile.path();
  g_log.notice() << "[DB] "
                 << "Temp MD Event file = " << tempFileName << "\n";

  // Construct a file
  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS" << std::endl;
  myfile << "x X m 100" << std::endl;
  myfile << "y Y m 100" << std::endl;
  myfile << "z Z m 100" << std::endl;
  myfile << "t T s 100" << std::endl;
  myfile << "# Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of "
            "coords" << std::endl;
  myfile << "MDEVENTS" << std::endl;

  double relruntime = 0;

  if (vec_ws2d.size() > 0) {
    Progress progress(this, 0, 1, vec_ws2d.size());
    size_t detindex = 0;
    for (auto it = vec_ws2d.begin(); it < vec_ws2d.end(); ++it) {
      std::size_t pos = std::distance(vec_ws2d.begin(), it);
      API::MatrixWorkspace_sptr thisWorkspace = *it;

      std::size_t nHist = thisWorkspace->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = thisWorkspace->getDetector(i);
        const MantidVec &signal = thisWorkspace->readY(i);
        const MantidVec &error = thisWorkspace->readE(i);
        myfile << signal[0] << " ";
        myfile << error[0] << " ";
        myfile << det->getID() + detindex << " ";
        myfile << pos << " ";
        Kernel::V3D detPos = det->getPos();
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        // Add a new dimension as event time
        /// TODO - Need to find out the duration of the run!
        relruntime += 30;
        myfile << relruntime << " ";
        myfile << std::endl;
      }

      // Increment on detector IDs
      if (nHist < 100)
        detindex += 100;
      else
        detindex += nHist;

      // Run time
      relruntime +=
          atof(thisWorkspace->run().getProperty("time")->value().c_str());

      progress.report("Creating MD WS");
    }
    myfile.close();
  }

  // Import to MD Workspace
  IAlgorithm_sptr importMDEWS = createChildAlgorithm("ImportMDEventWorkspace");
  // Now execute the Child Algorithm.
  try {
    importMDEWS->setPropertyValue("Filename", tempFileName);
    importMDEWS->setProperty("OutputWorkspace", "Test");
    importMDEWS->executeAsChildAlg();
  }
  catch (std::exception &exc) {
    throw std::runtime_error(
        std::string("Error running ImportMDEventWorkspace: ") + exc.what());
  }
  IMDEventWorkspace_sptr workspace =
      importMDEWS->getProperty("OutputWorkspace");
  if (!workspace)
    throw(std::runtime_error("Can not retrieve results of child algorithm "
                             "ImportMDEventWorkspace"));

  return workspace;
}

/**
 * @brief LoadHFIRPDD::createMonitorMDWorkspace
 * @param vec_ws2d
 * @param logvecmap
 * @return
 */
IMDEventWorkspace_sptr LoadHFIRPDData::createMonitorMDWorkspace(
    const std::vector<MatrixWorkspace_sptr> vec_ws2d,
    const std::map<std::string, std::vector<double> > logvecmap) {
  // Write the lsit of workspacs to a file to be loaded to an MD workspace
  Poco::TemporaryFile tmpFile;
  std::string tempFileName = tmpFile.path();
  g_log.notice() << "[DB] "
                 << "Temp MD Event file = " << tempFileName << "\n";

  // Construct a file
  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS" << std::endl;
  myfile << "x X m 100" << std::endl;
  myfile << "y Y m 100" << std::endl;
  myfile << "z Z m 100" << std::endl;
  myfile << "t T s 100" << std::endl;
  myfile << "# Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of "
            "coords" << std::endl;
  myfile << "MDEVENTS" << std::endl;

  double relruntime = 0;

  if (vec_ws2d.size() > 0) {
    Progress progress(this, 0, 1, vec_ws2d.size());
    size_t detindex = 0;
    for (auto it = vec_ws2d.begin(); it < vec_ws2d.end(); ++it) {
      std::size_t pos = std::distance(vec_ws2d.begin(), it);
      API::MatrixWorkspace_sptr thisWorkspace = *it;

      std::map<std::string, std::vector<double> >::const_iterator fiter;
      fiter = logvecmap.find("monitor");
      if (fiter == logvecmap.end())
        throw std::runtime_error(
            "Unable to find log 'monitor' in input workspace.");
      double signal = fiter->second[static_cast<size_t>(it - vec_ws2d.begin())];

      std::size_t nHist = thisWorkspace->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = thisWorkspace->getDetector(i);

        // const MantidVec &signal = thisWorkspace->readY(i);
        const MantidVec &error = thisWorkspace->readE(i);
        myfile << signal << " ";
        myfile << error[0] << " ";
        myfile << det->getID() + detindex << " ";
        myfile << pos << " ";
        Kernel::V3D detPos = det->getPos();
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        // Add a new dimension as event time
        myfile << relruntime << " ";
        myfile << std::endl;
      }

      // Increment on detector IDs
      if (nHist < 100)
        detindex += 100;
      else
        detindex += nHist;

      // Run time
      relruntime +=
          atof(thisWorkspace->run().getProperty("time")->value().c_str());

      progress.report("Creating MD WS");
    }
    myfile.close();
  }

  // Import to MD Workspace
  IAlgorithm_sptr importMDEWS = createChildAlgorithm("ImportMDEventWorkspace");
  // Now execute the Child Algorithm.
  try {
    importMDEWS->setPropertyValue("Filename", tempFileName);
    importMDEWS->setProperty("OutputWorkspace", "Test");
    importMDEWS->executeAsChildAlg();
  }
  catch (std::exception &exc) {
    throw std::runtime_error(
        std::string("Error running ImportMDEventWorkspace: ") + exc.what());
  }
  IMDEventWorkspace_sptr workspace =
      importMDEWS->getProperty("OutputWorkspace");
  if (!workspace)
    throw(std::runtime_error("Can not retrieve results of child algorithm "
                             "ImportMDEventWorkspace"));

  return workspace;
}

//---
/**
 * @brief LoadHFIRPDD::appendSampleLogs
 * @param mdws
 * @param logvecmap
 * @param vectimes
 */
void LoadHFIRPDData::appendSampleLogs(
    IMDEventWorkspace_sptr mdws,
    const std::map<std::string, std::vector<double> > &logvecmap,
    const std::vector<Kernel::DateAndTime> &vectimes) {
  // Check!
  size_t numexpinfo = mdws->getNumExperimentInfo();
  if (numexpinfo == 0)
    throw std::runtime_error(
        "There is no ExperimentInfo defined for MDWorkspace. "
        "It is impossible to add any log!");

  // Process the sample logs for MD workspace
  ExperimentInfo_sptr ei = mdws->getExperimentInfo(0);

  std::map<std::string, std::vector<double> >::const_iterator miter;
  for (miter = logvecmap.begin(); miter != logvecmap.end(); ++miter) {
    std::string logname = miter->first;
    const std::vector<double> &veclogval = miter->second;

    // Check log values and times
    if (veclogval.size() != vectimes.size()) {
      g_log.error() << "Log " << logname
                    << " has different number of log values ("
                    << veclogval.size() << ") than number of log entry time ("
                    << vectimes.size() << ")"
                    << "\n";
      continue;
    }

    // Create a new log
    TimeSeriesProperty<double> *templog =
        new TimeSeriesProperty<double>(logname);
    templog->addValues(vectimes, veclogval);

    // Add log to experiment info
    ei->mutableRun().addLogData(templog);
  }

  // MD workspace add experimental information
  mdws->addExperimentInfo(ei);

  return;
}

//---------------------------------------------------------------------------------
/** Append Experiment Info
 * @brief LoadHFIRPDD::addExperimentInfos
 * @param mdwd
 * @param vec_ws2d
 */
void LoadHFIRPDData::addExperimentInfos(
    API::IMDEventWorkspace_sptr mdws,
    const std::vector<API::MatrixWorkspace_sptr> vec_ws2d,
    const int &init_runnumber) {
  for (size_t i = 0; i < vec_ws2d.size(); ++i) {
    // Create an ExperimentInfo object
    ExperimentInfo_sptr tmp_expinfo = boost::make_shared<ExperimentInfo>();
    Geometry::Instrument_const_sptr tmp_inst = vec_ws2d[i]->getInstrument();
    tmp_expinfo->setInstrument(tmp_inst);

    tmp_expinfo->mutableRun().addProperty(new PropertyWithValue<int>(
        "run_number", static_cast<int>(i) + init_runnumber));

    // Add ExperimentInfo to workspace
    mdws->addExperimentInfo(tmp_expinfo);
  }

  return;
}
} // namespace DataHandling
} // namespace Mantid
