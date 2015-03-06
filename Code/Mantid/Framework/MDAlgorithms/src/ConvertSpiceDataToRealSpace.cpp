#include "MantidMDAlgorithms/ConvertSpiceDataToRealSpace.h"

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"

#include <boost/algorithm/string/predicate.hpp>
#include <Poco/TemporaryFile.h>

namespace Mantid {
namespace MDAlgorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

DECLARE_ALGORITHM(ConvertSpiceDataToRealSpace)

//------------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertSpiceDataToRealSpace::ConvertSpiceDataToRealSpace()
    : m_instrumentName(""), m_numSpec(0) {}

//------------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertSpiceDataToRealSpace::~ConvertSpiceDataToRealSpace() {}

//----------------------------------------------------------------------------------------------
/** Init
 */
void ConvertSpiceDataToRealSpace::init() {
  declareProperty(new WorkspaceProperty<TableWorkspace>("InputWorkspace", "",
                                                        Direction::Input),
                  "Input table workspace for data.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("RunInfoWorkspace", "",
                                                         Direction::Input),
                  "Input matrix workspace containing sample logs.  "
                  "It can be the RunInfoWorkspace output from LoadSpiceAscii. "
                  "It serves as parent workspace in the algorithm.");

  declareProperty("RunStart", "",
                  "User specified run start time of the experiment "
                  "in case that the run start time is not specified in the "
                  "input RunInfoWorkspace.");

  /// TODO - Add HB2B as it is implemented in future
  std::vector<std::string> allowedinstruments;
  allowedinstruments.push_back("HB2A");
  auto instrumentvalidator =
      boost::make_shared<ListValidator<std::string> >(allowedinstruments);
  declareProperty("Instrument", "HB2A", instrumentvalidator,
                  "Instrument to be loaded. ");

  declareProperty("DetectorPrefix", "anode",
                  "Prefix of the name for detectors. ");

  declareProperty("RunNumberName", "Pt.",
                  "Log name for run number/measurement point.");

  declareProperty(
      "RotationAngleLogName", "2theta",
      "Log name for rotation angle as the 2theta value of detector 0.");

  declareProperty(
      "MonitorCountsLogName", "monitor",
      "Name of the sample log to record monitor counts of each run.");

  declareProperty("DurationLogName", "time",
                  "Name of the sample log to record the duration of each run.");

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
void ConvertSpiceDataToRealSpace::exec() {

  // Process inputs
  DataObjects::TableWorkspace_sptr dataTableWS = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr parentWS = getProperty("RunInfoWorkspace");
  m_instrumentName = getPropertyValue("Instrument");

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
    }
    catch (...) {
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
    }
    catch (...) {
      g_log.warning() << "RunStart from input property is not correct. "
                      << "It cannot be convert from '" << runstartstr << "'."
                      << "\n";
    }
  }

  if (!hasrunstartset) {
    g_log.warning("Run-start time is not defined either in "
                  "input parent workspace or given by user. 1990-01-01 "
                  "00:00:01 is used");
  }

  // Convert table workspace to a list of 2D workspaces
  std::map<std::string, std::vector<double> > logvecmap;
  std::vector<Kernel::DateAndTime> vectimes;
  std::vector<MatrixWorkspace_sptr> vec_ws2d =
      convertToWorkspaces(dataTableWS, parentWS, runstart, logvecmap, vectimes);

  // Convert to MD workspaces
  g_log.debug("About to converting to workspaces done!");
  IMDEventWorkspace_sptr m_mdEventWS = convertToMDEventWS(vec_ws2d);
  std::string monitorlogname = getProperty("MonitorCountsLogName");
  IMDEventWorkspace_sptr mdMonitorWS =
      createMonitorMDWorkspace(vec_ws2d, logvecmap[monitorlogname]);

  // Add experiment info for each run and sample log to the first experiment
  // info object
  addExperimentInfos(m_mdEventWS, vec_ws2d);
  addExperimentInfos(mdMonitorWS, vec_ws2d);
  appendSampleLogs(m_mdEventWS, logvecmap, vectimes);

  // Set property
  setProperty("OutputWorkspace", m_mdEventWS);
  setProperty("OutputMonitorWorkspace", mdMonitorWS);
}

//----------------------------------------------------------------------------------------------
/** Convert runs/pts from table workspace to a list of workspace 2D
 * @brief ConvertSpiceDataToRealSpace::convertToWorkspaces
 * @param tablews
 * @param parentws
 * @param runstart
 * @param logvecmap
 * @param vectimes
 * @return
 */
std::vector<MatrixWorkspace_sptr>
ConvertSpiceDataToRealSpace::convertToWorkspaces(
    DataObjects::TableWorkspace_sptr tablews,
    API::MatrixWorkspace_const_sptr parentws, Kernel::DateAndTime runstart,
    std::map<std::string, std::vector<double> > &logvecmap,
    std::vector<Kernel::DateAndTime> &vectimes) {
  // Get table workspace's column information
  size_t ipt, irotangle, itime;
  std::vector<std::pair<size_t, size_t> > anodelist;
  std::map<std::string, size_t> sampleindexlist;
  readTableInfo(tablews, ipt, irotangle, itime, anodelist, sampleindexlist);
  m_numSpec = anodelist.size();

  // Load data
  size_t numws = tablews->rowCount();
  std::vector<MatrixWorkspace_sptr> vecws(numws);
  double duration = 0;
  vectimes.resize(numws);
  for (size_t irow = 0; irow < numws; ++irow) {
    vecws[irow] = loadRunToMatrixWS(tablews, irow, parentws, runstart, ipt,
                                    irotangle, itime, anodelist, duration);
    vectimes[irow] = runstart;
    runstart += static_cast<int64_t>(duration * 1.0E9);
  }

  // Process log data which will not be put to matrix workspace but will got to
  // MDWorkspace
  parseSampleLogs(tablews, sampleindexlist, logvecmap);

  g_log.debug() << "Number of matrix workspaces in vector = " << vecws.size()
                << "\n";
  return vecws;
}

//------------------------------------------------------------------------------------------------
/** Parse sample logs from table workspace and return with a set of vectors
 * @brief ConvertSpiceDataToRealSpace::parseSampleLogs
 * @param tablews
 * @param indexlist
 * @param logvecmap
 */
void ConvertSpiceDataToRealSpace::parseSampleLogs(
    DataObjects::TableWorkspace_sptr tablews,
    const std::map<std::string, size_t> &indexlist,
    std::map<std::string, std::vector<double> > &logvecmap) {
  size_t numrows = tablews->rowCount();

  std::map<std::string, size_t>::const_iterator indexiter;
  for (indexiter = indexlist.begin(); indexiter != indexlist.end();
       ++indexiter) {
    std::string logname = indexiter->first;
    size_t icol = indexiter->second;

    g_log.information() << " Parsing log " << logname << "\n";

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
    DataObjects::TableWorkspace_sptr tablews, size_t irow,
    MatrixWorkspace_const_sptr parentws, Kernel::DateAndTime runstart,
    size_t ipt, size_t irotangle, size_t itime,
    const std::vector<std::pair<size_t, size_t> > anodelist, double &duration) {
  // New workspace from parent workspace
  MatrixWorkspace_sptr tempws =
      WorkspaceFactory::Instance().create(parentws, m_numSpec, 2, 1);

  // Set up angle, time and run number
  double twotheta = tablews->cell<double>(irow, irotangle);
  TimeSeriesProperty<double> *prop2theta =
      new TimeSeriesProperty<double>("rotangle");

  prop2theta->addValue(runstart, twotheta);
  tempws->mutableRun().addProperty(prop2theta);

  TimeSeriesProperty<std::string> *proprunstart =
      new TimeSeriesProperty<std::string>("run_start");
  proprunstart->addValue(runstart, runstart.toISO8601String());

  g_log.debug() << "Run " << irow << ": set run start to "
                << runstart.toISO8601String() << "\n";
  if (tempws->run().hasProperty("run_start")) {
    g_log.information() << "Temporary workspace inherites run_start as "
                        << tempws->run().getProperty("run_start")->value()
                        << ". It will be replaced by the correct value. "
                        << "\n";
    tempws->mutableRun().removeProperty("run_start");
  }
  tempws->mutableRun().addProperty(proprunstart);

  int pt = tablews->cell<int>(irow, ipt);
  tempws->mutableRun().addProperty(
      new PropertyWithValue<int>("run_number", pt));

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
    double yvalue = tablews->cell<double>(irow, anodelist[i].second);
    tempws->dataY(i)[0] = yvalue;
    if (yvalue >= 1)
      tempws->dataE(i)[0] = sqrt(yvalue);
    else
      tempws->dataE(i)[0] = 1;
  }

  // Return duration
  duration = tablews->cell<double>(irow, itime);

  return tempws;
}

//----------------------------------------------------------------------------------------------
/** Read table workspace's column information
 * @brief ConvertSpiceDataToRealSpace::readTableInfo
 * @param tablews
 * @param ipt
 * @param irotangle
 * @param itime
 * @param anodelist
 * @param samplenameindexmap
 */
void ConvertSpiceDataToRealSpace::readTableInfo(
    TableWorkspace_const_sptr tablews, size_t &ipt, size_t &irotangle,
    size_t &itime, std::vector<std::pair<size_t, size_t> > &anodelist,
    std::map<std::string, size_t> &samplenameindexmap) {

  // Get detectors' names and other sample names
  std::string anodelogprefix = getProperty("DetectorPrefix");
  const std::vector<std::string> &colnames = tablews->getColumnNames();
  for (size_t icol = 0; icol < colnames.size(); ++icol) {
    const std::string &colname = colnames[icol];

    if (boost::starts_with(colname, anodelogprefix)) {
      // anode
      std::vector<std::string> terms;
      boost::split(terms, colname, boost::is_any_of(anodelogprefix));
      size_t anodeid = static_cast<size_t>(atoi(terms.back().c_str()));
      anodelist.push_back(std::make_pair(anodeid, icol));
    } else {
      samplenameindexmap.insert(std::make_pair(colname, icol));
    }
  } // ENDFOR (icol)

  // Check detectors' names
  if (anodelist.size() == 0) {
    std::stringstream errss;
    errss << "There is no log name starting with " << anodelogprefix
          << " for detector. ";
    throw std::runtime_error(errss.str());
  }

  // Find out other essential sample log names
  std::map<std::string, size_t>::iterator mapiter;

  std::string ptname = getProperty("RunNumberName");                 // "Pt."
  std::string monitorlogname = getProperty("MonitorCountsLogName");  //"monitor"
  std::string durationlogname = getProperty("DurationLogName");      //"time"
  std::string rotanglelogname = getProperty("RotationAngleLogName"); // "2theta"

  std::vector<std::string> lognames;
  lognames.push_back(ptname);
  lognames.push_back(monitorlogname);
  lognames.push_back(durationlogname);
  lognames.push_back(rotanglelogname);

  std::vector<size_t> ilognames(lognames.size());

  for (size_t i = 0; i < lognames.size(); ++i) {
    const std::string &logname = lognames[i];
    mapiter = samplenameindexmap.find(logname);
    if (mapiter != samplenameindexmap.end()) {
      ilognames[i] = mapiter->second;
    } else {
      std::stringstream ess;
      ess << "Essential log name " << logname
          << " cannot be found in data table workspace.";
      throw std::runtime_error(ess.str());
    }
  }

  // Retrieve the vector index
  ipt = ilognames[0];
  itime = ilognames[2];
  irotangle = ilognames[3];

  // Sort out anode id index list;
  std::sort(anodelist.begin(), anodelist.end());

  return;
}

//----------------------------------------------------------------------------------------------

/** Convert to MD Event workspace
 * @brief ConvertSpiceDataToRealSpace::convertToMDEventWS
 * @param vec_ws2d
 * @return
 */
IMDEventWorkspace_sptr ConvertSpiceDataToRealSpace::convertToMDEventWS(
    const std::vector<MatrixWorkspace_sptr> &vec_ws2d) {
  // Write the lsit of workspacs to a file to be loaded to an MD workspace
  Poco::TemporaryFile tmpFile;
  std::string tempFileName = tmpFile.path();
  g_log.debug() << "Creating temporary MD Event file = " << tempFileName
                << "\n";

  // Construct a file
  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS" << std::endl;
  myfile << "x X m 100" << std::endl;
  myfile << "y Y m 100" << std::endl;
  myfile << "z Z m 100" << std::endl;
  myfile << "# Signal, Error, RunId, DetectorId, coord1, coord2, ... to end of "
            "coords" << std::endl;
  myfile << "MDEVENTS" << std::endl;

  if (vec_ws2d.size() > 0) {
    Progress progress(this, 0, 1, vec_ws2d.size());
    size_t detindex = 0;
    for (auto it = vec_ws2d.begin(); it < vec_ws2d.end(); ++it) {
      API::MatrixWorkspace_sptr thisWorkspace = *it;
      int runnumber =
          atoi(thisWorkspace->run().getProperty("run_number")->value().c_str());

      std::size_t nHist = thisWorkspace->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = thisWorkspace->getDetector(i);
        const MantidVec &signal = thisWorkspace->readY(i);
        const MantidVec &error = thisWorkspace->readE(i);
        myfile << signal[0] << " ";
        myfile << error[0] << " ";
        myfile << runnumber << " ";
        myfile << det->getID() + detindex << " ";
        Kernel::V3D detPos = det->getPos();
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        myfile << std::endl;
      }

      // Increment on detector IDs
      detindex += nHist;

      progress.report("Creating MD WS");
    }
    myfile.close();
  } else {
    throw std::runtime_error(
        "There is no MatrixWorkspace to construct MDWorkspace.");
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

//-----------------------------------------------------------------------------------------------
/** Create an MDWorkspace for monitoring counts.
 * @brief LoadHFIRPDD::createMonitorMDWorkspace
 * @param vec_ws2d
 * @param vecmonitor
 * @return
 */
IMDEventWorkspace_sptr ConvertSpiceDataToRealSpace::createMonitorMDWorkspace(
    const std::vector<MatrixWorkspace_sptr> vec_ws2d,
    const std::vector<double> &vecmonitor) {
  // Write the lsit of workspacs to a file to be loaded to an MD workspace
  Poco::TemporaryFile tmpFile;
  std::string tempFileName = tmpFile.path();
  g_log.debug() << "Creating temporary MD Event file for monitor counts = "
                << tempFileName << "\n";

  // Construct a file
  std::ofstream myfile;
  myfile.open(tempFileName.c_str());
  myfile << "DIMENSIONS" << std::endl;
  myfile << "x X m 100" << std::endl;
  myfile << "y Y m 100" << std::endl;
  myfile << "z Z m 100" << std::endl;
  myfile << "# Signal, Error, RunId, coord1, DetectorId, coord2, ... to end of "
            "coords" << std::endl;
  myfile << "MDEVENTS" << std::endl;

  if (vec_ws2d.size() > 0) {
    Progress progress(this, 0, 1, vec_ws2d.size());
    size_t detindex = 0;
    for (auto it = vec_ws2d.begin(); it < vec_ws2d.end(); ++it) {
      API::MatrixWorkspace_sptr thisWorkspace = *it;
      int runnumber =
          atoi(thisWorkspace->run().getProperty("run_number")->value().c_str());

      double signal = vecmonitor[static_cast<size_t>(it - vec_ws2d.begin())];

      std::size_t nHist = thisWorkspace->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i) {
        Geometry::IDetector_const_sptr det = thisWorkspace->getDetector(i);

        // const MantidVec &signal = thisWorkspace->readY(i);
        const MantidVec &error = thisWorkspace->readE(i);
        myfile << signal << " ";
        myfile << error[0] << " ";
        myfile << runnumber << " ";
        myfile << det->getID() + detindex << " ";
        Kernel::V3D detPos = det->getPos();
        myfile << detPos.X() << " ";
        myfile << detPos.Y() << " ";
        myfile << detPos.Z() << " ";
        myfile << std::endl;
      }

      // Increment on detector IDs
      detindex += nHist;

      progress.report("Creating MD WS");
    }
    myfile.close();
  } else {
    throw std::runtime_error(
        "There is no MatrixWorkspace to construct MDWorkspace.");
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

//-----------------------------------------------------------------------------------------------
/** Create sample logs for MD workspace
 * @brief LoadHFIRPDD::appendSampleLogs
 * @param mdws
 * @param logvecmap
 * @param vectimes
 */
void ConvertSpiceDataToRealSpace::appendSampleLogs(
    IMDEventWorkspace_sptr mdws,
    const std::map<std::string, std::vector<double> > &logvecmap,
    const std::vector<Kernel::DateAndTime> &vectimes) {
  // Check!
  size_t numexpinfo = mdws->getNumExperimentInfo();
  if (numexpinfo == 0)
    throw std::runtime_error(
        "There is no ExperimentInfo defined for MDWorkspace. "
        "It is impossible to add any log!");
  else if (numexpinfo != vectimes.size() + 1)
    throw std::runtime_error(
        "The number of ExperimentInfo should be 1 more than "
        "the length of vector of time, i.e., number of matrix workspaces.");

  std::map<std::string, std::vector<double> >::const_iterator miter;

  // get runnumber vector
  std::string runnumlogname = getProperty("RunNumberName");
  miter = logvecmap.find(runnumlogname);
  if (miter == logvecmap.end())
    throw std::runtime_error("Impossible not to find Pt. in log vec map.");
  const std::vector<double> &vecrunno = miter->second;

  // Add run_start and start_time to each ExperimentInfo
  for (size_t i = 0; i < vectimes.size(); ++i) {
    Kernel::DateAndTime runstart = vectimes[i];
    mdws->getExperimentInfo(static_cast<uint16_t>(i))->mutableRun().addLogData(
        new PropertyWithValue<std::string>("run_start",
                                           runstart.toFormattedString()));
  }
  mdws->getExperimentInfo(static_cast<uint16_t>(vectimes.size()))
      ->mutableRun()
      .addLogData(new PropertyWithValue<std::string>(
           "run_start", vectimes[0].toFormattedString()));

  // Add sample logs
  // get hold of last experiment info
  ExperimentInfo_sptr eilast =
      mdws->getExperimentInfo(static_cast<uint16_t>(numexpinfo - 1));

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

    // For N single value experiment info
    for (uint16_t i = 0; i < static_cast<uint16_t>(veclogval.size()); ++i) {
      // get ExperimentInfo
      ExperimentInfo_sptr tmpei = mdws->getExperimentInfo(i);
      // check run number matches
      int runnumber =
          atoi(tmpei->run().getProperty("run_number")->value().c_str());
      if (runnumber != static_cast<int>(vecrunno[i]))
        throw std::runtime_error("Run number does not match to Pt. value.");
      // add property
      tmpei->mutableRun().addLogData(
          new PropertyWithValue<double>(logname, veclogval[i]));
    }

    // Create a new log
    TimeSeriesProperty<double> *templog =
        new TimeSeriesProperty<double>(logname);
    templog->addValues(vectimes, veclogval);

    // Add log to experiment info
    eilast->mutableRun().addLogData(templog);
  }

  return;
}

//---------------------------------------------------------------------------------
/** Add Experiment Info to the MDWorkspace.  Add 1+N ExperimentInfo
 * @brief ConvertSpiceDataToRealSpace::addExperimentInfos
 * @param mdws
 * @param vec_ws2d
 */
void ConvertSpiceDataToRealSpace::addExperimentInfos(
    API::IMDEventWorkspace_sptr mdws,
    const std::vector<API::MatrixWorkspace_sptr> vec_ws2d) {
  // Add N experiment info as there are N measurment points
  for (size_t i = 0; i < vec_ws2d.size(); ++i) {
    // Create an ExperimentInfo object
    ExperimentInfo_sptr tmp_expinfo = boost::make_shared<ExperimentInfo>();
    Geometry::Instrument_const_sptr tmp_inst = vec_ws2d[i]->getInstrument();
    tmp_expinfo->setInstrument(tmp_inst);

    int runnumber =
        atoi(vec_ws2d[i]->run().getProperty("run_number")->value().c_str());
    tmp_expinfo->mutableRun().addProperty(
        new PropertyWithValue<int>("run_number", runnumber));

    // Add ExperimentInfo to workspace
    mdws->addExperimentInfo(tmp_expinfo);
  }

  // Add one additional in order to contain the combined sample logs
  ExperimentInfo_sptr combine_expinfo = boost::make_shared<ExperimentInfo>();
  combine_expinfo->mutableRun().addProperty(
      new PropertyWithValue<int>("run_number", -1));
  mdws->addExperimentInfo(combine_expinfo);

  return;
}
} // namespace DataHandling
} // namespace Mantid
