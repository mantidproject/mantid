//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidNexus/MuonNexusReader.h"
#include "MantidNexus/NexusClasses.h"

#include <Poco/Path.h>
#include <limits>
#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace DataHandling {
using namespace DataObjects;

// Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMuonNexus1);

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using namespace Mantid::NeXus;

/// Empty default constructor
LoadMuonNexus1::LoadMuonNexus1() : LoadMuonNexus() {}

/** Executes the algorithm. Reading in the file and creating and populating
*  the output workspace
*
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid
*values
*/
void LoadMuonNexus1::exec() {
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");
  // Retrieve the entry number
  m_entrynumber = getProperty("EntryNumber");

  NXRoot root(m_filename);
  NXEntry entry = root.openEntry("run/histogram_data_1");
  try {
    NXInfo info = entry.getDataSetInfo("time_zero");
    if (info.stat != NX_ERROR) {
      double dum = root.getFloat("run/histogram_data_1/time_zero");
      setProperty("TimeZero", dum);
    }
  } catch (...) {
  }

  try {
    NXInfo infoResolution = entry.getDataSetInfo("resolution");
    NXInt counts = root.openNXInt("run/histogram_data_1/counts");
    std::string firstGoodBin = counts.attributes("first_good_bin");
    if (!firstGoodBin.empty() && infoResolution.stat != NX_ERROR) {
      double resolution;

      switch (infoResolution.type) {
      case NX_FLOAT32:
        resolution = static_cast<double>(entry.getFloat("resolution"));
        break;
      case NX_INT32:
        resolution = static_cast<double>(entry.getInt("resolution"));
        break;
      default:
        throw std::runtime_error("Unsupported data type for resolution");
      }

      double bin = static_cast<double>(boost::lexical_cast<int>(firstGoodBin));
      double bin_size = resolution / 1000000.0;

      setProperty("FirstGoodData", bin * bin_size);
    }
  } catch (std::exception &e) {
    g_log.warning() << "Error while loading the FirstGoodData value: "
                    << e.what() << "\n";
  }

  NXEntry nxRun = root.openEntry("run");
  std::string title;
  std::string notes;
  try {
    title = nxRun.getString("title");
    notes = nxRun.getString("notes");
  } catch (...) {
  }
  std::string run_num;
  try {
    run_num = boost::lexical_cast<std::string>(nxRun.getInt("number"));
  } catch (...) {
  }

  MuonNexusReader nxload;
  nxload.readFromFile(m_filename);

  // Read in the instrument name from the Nexus file
  m_instrument_name = nxload.getInstrumentName();
  // Read in the number of spectra in the Nexus file
  m_numberOfSpectra = nxload.t_nsp1;
  if (m_entrynumber != 0) {
    m_numberOfPeriods = 1;
    if (m_entrynumber > nxload.t_nper) {
      throw std::invalid_argument("Invalid Entry Number:Enter a valid number");
    }
  } else {
    // Read the number of periods in this file
    m_numberOfPeriods = nxload.t_nper;
  }

  bool autoGroup = getProperty("AutoGroup");

  // Grouping info should be returned if user has set the property
  bool returnGrouping = !getPropertyValue("DetectorGroupingTable").empty();

  // Call private method to validate the optional parameters, if set
  checkOptionalProperties();
  // Calculate the size of a workspace, given its number of periods & spectra to
  // read
  int64_t total_specs;
  if (m_interval || m_list) {
    // Remove from list possible duplicate specs
    for (auto it=m_spec_list.begin(); it!=m_spec_list.end(); ) {
      if ( (*it>=m_spec_min) && (*it<=m_spec_max) ) {
        it = m_spec_list.erase(it);
      } else {
        ++it;
      }
    }
    total_specs = m_spec_list.size();
    if (m_interval) {
      total_specs += (m_spec_max - m_spec_min + 1);
      m_spec_max += 1;
    }
  } else {
    total_specs = m_numberOfSpectra;
    // for nexus return all spectra
    m_spec_min = 1;
    m_spec_max = m_numberOfSpectra+1; // Add +1 to iterate
  }


  Workspace_sptr loadedGrouping;

  // Try to load detector grouping info, if needed for auto-grouping or user
  // requested it
  if (autoGroup || returnGrouping) {
    loadedGrouping = loadDetectorGrouping(root);

    if (loadedGrouping && returnGrouping) {
      // Return loaded grouping, if requested
      setProperty("DetectorGroupingTable", loadedGrouping);
    }

    if (!loadedGrouping && autoGroup) {
      // If autoGroup requested and no grouping in the file - show a warning
      g_log.warning(
          "Unable to load grouping from the file. Grouping not applied.");
    }
  }

  // If multiperiod, will need to hold the Instrument & Sample for copying
  boost::shared_ptr<Instrument> instrument;
  boost::shared_ptr<Sample> sample;

  // Read the number of time channels (i.e. bins) from the Nexus file
  const int channelsPerSpectrum = nxload.t_ntc1;
  // Read in the time bin boundaries
  const int lengthIn = channelsPerSpectrum + 1;

  // Try to load dead time info
  loadDeadTimes(root);

  // Create the 2D workspace for the output
  DataObjects::Workspace2D_sptr localWorkspace =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", total_specs,
                                              lengthIn, lengthIn - 1));
  localWorkspace->setTitle(title);
  localWorkspace->setComment(notes);
  localWorkspace->mutableRun().addLogData(
      new PropertyWithValue<std::string>("run_number", run_num));
  // Set the unit on the workspace to muon time, for now in the form of a Label
  // Unit
  boost::shared_ptr<Kernel::Units::Label> lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Units::Symbol::Microsecond);
  localWorkspace->getAxis(0)->unit() = lblUnit;
  // Set y axis unit
  localWorkspace->setYUnit("Counts");

  WorkspaceGroup_sptr wsGrpSptr = WorkspaceGroup_sptr(new WorkspaceGroup);

  API::Progress progress(this, 0., 1., m_numberOfPeriods * total_specs);
  // Loop over the number of periods in the Nexus file, putting each period in a
  // separate workspace
  for (int64_t period = 0; period < m_numberOfPeriods; ++period) {
    if (m_entrynumber != 0) {
      period = m_entrynumber - 1;
      if (period != 0) {
        loadRunDetails(localWorkspace);
        runLoadInstrument(localWorkspace);
      }
    }

    if (period == 0) {
      // Only run the Child Algorithms once
      loadRunDetails(localWorkspace);
      runLoadInstrument(localWorkspace);
      runLoadLog(localWorkspace);
      localWorkspace->populateInstrumentParameters();
    } else // We are working on a higher period of a multiperiod raw file
    {
      localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create(localWorkspace));
      localWorkspace->setTitle(title);
      localWorkspace->setComment(notes);
    }

    size_t counter = 0;
    for (int64_t i = m_spec_min; i < m_spec_max; ++i) {
      // Shift the histogram to read if we're not in the first period
      specid_t histToRead = static_cast<specid_t>(i-1 + period * nxload.t_nsp1);
      specid_t specNo = static_cast<specid_t>(i);
      loadData(counter, histToRead, specNo, nxload, lengthIn - 1,
               localWorkspace); // added -1 for NeXus
      counter++;
      progress.report();
    }
    // Read in the spectra in the optional list parameter, if set
    if (m_list) {
      for (size_t i = 0; i < m_spec_list.size(); ++i) {
        specid_t histToRead = static_cast<specid_t>(m_spec_list[i]-1 + period * nxload.t_nsp1);
        specid_t specNo = static_cast<specid_t>(m_spec_list[i]);
        loadData(counter, histToRead, specNo, nxload, lengthIn - 1,
          localWorkspace);
        counter++;
        progress.report();
      }
    }
    // Just a sanity check
    assert(counter == size_t(total_specs));

    Workspace_sptr outWs;

    if (autoGroup && loadedGrouping) {
      TableWorkspace_sptr groupingTable;

      if (auto table =
              boost::dynamic_pointer_cast<TableWorkspace>(loadedGrouping)) {
        groupingTable = table;
      } else if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
                     loadedGrouping)) {
        groupingTable =
            boost::dynamic_pointer_cast<TableWorkspace>(group->getItem(period));
      }

      Algorithm_sptr groupDet = createChildAlgorithm("MuonGroupDetectors");
      groupDet->setProperty("InputWorkspace", localWorkspace);
      groupDet->setProperty("DetectorGroupingTable", groupingTable);
      groupDet->execute();

      MatrixWorkspace_sptr groupedWs = groupDet->getProperty("OutputWorkspace");

      outWs = groupedWs;
    } else {
      outWs = localWorkspace;
    }

    if (m_numberOfPeriods == 1)
      setProperty("OutputWorkspace", outWs);
    else
      // In case of multiple periods, just add workspace to the group, and we
      // will return the
      // group later
      wsGrpSptr->addWorkspace(outWs);

  } // loop over periods

  if (m_numberOfPeriods > 1) {
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
  }

}

/**
 * Loads dead time table for the detector.
 * @param root :: Root entry of the Nexus to read dead times from
 */
void LoadMuonNexus1::loadDeadTimes(NXRoot &root) {
  // If dead times workspace name is empty - caller doesn't need dead times
  if (getPropertyValue("DeadTimeTable").empty())
    return;

  NXEntry detector = root.openEntry("run/instrument/detector");

  NXInfo infoDeadTimes = detector.getDataSetInfo("deadtimes");
  if (infoDeadTimes.stat != NX_ERROR) {
    NXFloat deadTimesData = detector.openNXFloat("deadtimes");
    deadTimesData.load();

    int numDeadTimes = deadTimesData.dim0();

    std::vector<int> specToLoad;
    std::vector<double> deadTimes;

    // Set the spectrum list that should be loaded
    if ( m_interval || m_list ) {
      // Load only selected spectra
      for (int64_t i=m_spec_min; i<m_spec_max; i++) {
        specToLoad.push_back(static_cast<int>(i));
      }
      for (auto it=m_spec_list.begin(); it!=m_spec_list.end(); ++it) {
        specToLoad.push_back(*it);
      }
    } else {
      // Load all the spectra
      // Start from 1 to N+1 to be consistent with 
      // the case where spectra are specified
      for (int i=1; i<numDeadTimes/m_numberOfPeriods+1; i++)
        specToLoad.push_back(i);
    }


    if (numDeadTimes < m_numberOfSpectra) {
      // Check number of dead time entries match the number of 
      // spectra in the nexus file
      throw Exception::FileError(
        "Number of dead times specified is less than number of spectra",
        m_filename);

    } else if (numDeadTimes % m_numberOfSpectra) {

      // At least, number of dead times should cover the number of spectra
      throw Exception::FileError(
        "Number of dead times doesn't cover every spectra in every period",
        m_filename);
    } else {

      if ( m_numberOfPeriods == 1 ) {
        // Simpliest case - one dead time for one detector

        // Populate deadTimes
        for (auto it=specToLoad.begin(); it!=specToLoad.end(); ++it) {
          deadTimes.push_back(deadTimesData[*it-1]);
        }
        // Load into table
        TableWorkspace_sptr table = createDeadTimeTable(specToLoad, deadTimes);
        setProperty("DeadTimeTable", table);

      } else {
        // More complex case - different dead times for different periods

        WorkspaceGroup_sptr tableGroup = boost::make_shared<WorkspaceGroup>();

        for (int64_t i=0; i<m_numberOfPeriods; i++) {

          // Populate deadTimes
          for (auto it=specToLoad.begin(); it!=specToLoad.end(); ++it) {
            int index = static_cast<int>(*it -1 + i*m_numberOfSpectra);
            deadTimes.push_back(deadTimesData[index]);
          }

          // Load into table
          TableWorkspace_sptr table = createDeadTimeTable(specToLoad,deadTimes);

          tableGroup->addWorkspace(table);
        }

        setProperty("DeadTimeTable", tableGroup);
      }
    }
  }
  // It is expected that file might not contain any dead times, so not finding
  // them is not an
  // error
}

/**
 * Loads detector grouping.
 * @param root :: Root entry of the Nexus file to read from
 */
Workspace_sptr LoadMuonNexus1::loadDetectorGrouping(NXRoot &root) {
  NXEntry dataEntry = root.openEntry("run/histogram_data_1");

  NXInfo infoGrouping = dataEntry.getDataSetInfo("grouping");
  if (infoGrouping.stat != NX_ERROR) {
    NXInt groupingData = dataEntry.openNXInt("grouping");
    groupingData.load();

    int numGroupingEntries = groupingData.dim0();

    std::vector<int> specToLoad;
    std::vector<int> grouping;

    // Set the spectrum list that should be loaded
    if ( m_interval || m_list ) {
      // Load only selected spectra
      for (int64_t i=m_spec_min; i<m_spec_max; i++) {
        specToLoad.push_back(static_cast<int>(i));
      }
      for (auto it=m_spec_list.begin(); it!=m_spec_list.end(); ++it) {
        specToLoad.push_back(*it);
      }
    } else {
      // Load all the spectra
      // Start from 1 to N+1 to be consistent with 
      // the case where spectra are specified
      for (int i=1; i<m_numberOfSpectra+1; i++)
        specToLoad.push_back(i);
    }

    if (numGroupingEntries < m_numberOfSpectra) {
      // Check number of dead time entries match the number of 
      // spectra in the nexus file
      throw Exception::FileError(
          "Number of grouping entries is less than number of spectra",
          m_filename);

    } else if (numGroupingEntries % m_numberOfSpectra) {
      // At least the number of entries should cover all the spectra
      throw Exception::FileError("Number of grouping entries doesn't cover "
        "every spectra in every period",
        m_filename);

    } else {

      if ( m_numberOfPeriods==1 ) {
        // Simpliest case - one grouping entry per spectra

        for (auto it=specToLoad.begin(); it!=specToLoad.end(); ++it) {
          int index = *it - 1 + static_cast<int>((m_entrynumber-1) * m_numberOfSpectra);
          grouping.push_back(groupingData[index]);
        }

        TableWorkspace_sptr table =
          createDetectorGroupingTable(grouping.begin(), grouping.end());

        if (table->rowCount() != 0)
          return table;

      } else {
        // More complex case - grouping information for every period

        WorkspaceGroup_sptr tableGroup = boost::make_shared<WorkspaceGroup>();

        for (int i=0; i<m_numberOfPeriods; i++) {

          // Get the grouping
          grouping.clear();
          for (auto it=specToLoad.begin(); it!=specToLoad.end(); ++it) {
            int index = *it - 1 + i * static_cast<int>(m_numberOfSpectra);
            grouping.push_back(groupingData[index]);
          }

          // Set table for period i
          TableWorkspace_sptr table =
            createDetectorGroupingTable(grouping.begin(), grouping.end());

          // Add table to group
          if (table->rowCount() != 0)
            tableGroup->addWorkspace(table);
        }

        if (tableGroup->size() != 0) {
          if (tableGroup->size() != static_cast<size_t>(m_numberOfPeriods))
            throw Exception::FileError("Zero grouping for some of the periods",
            m_filename);

          return tableGroup;
        }
      }
    }
  }
  return Workspace_sptr();
}

/**
 * Creates Dead Time Table using all the data between begin and end.
 * @param specToLoad :: vector containing the spectrum numbers to load
 * @param deadTimes :: vector containing the corresponding dead times
 * @return Dead Time Table create using the data
 */
TableWorkspace_sptr
LoadMuonNexus1::createDeadTimeTable(std::vector<int> specToLoad,
                                    std::vector<double> deadTimes) {
  TableWorkspace_sptr deadTimeTable =
      boost::dynamic_pointer_cast<TableWorkspace>(
          WorkspaceFactory::Instance().createTable("TableWorkspace"));

  deadTimeTable->addColumn("int", "spectrum");
  deadTimeTable->addColumn("double", "dead-time");

  for (size_t i = 0; i<specToLoad.size(); i++) {
    TableRow row = deadTimeTable->appendRow();
    row << specToLoad[i] << deadTimes[i];
  }

  return deadTimeTable;
}

/**
 * Creates Detector Grouping Table using all the data between begin and end.
 *
 * @param begin :: Iterator to the first element of the data to use
 * @param   end :: Iterator to the last element of the data to use
 * @return Detector Grouping Table create using the data
 */
TableWorkspace_sptr LoadMuonNexus1::createDetectorGroupingTable(
    std::vector<int>::const_iterator begin,
    std::vector<int>::const_iterator end) {
  auto detectorGroupingTable = boost::dynamic_pointer_cast<TableWorkspace>(
      WorkspaceFactory::Instance().createTable("TableWorkspace"));

  detectorGroupingTable->addColumn("vector_int", "Detectors");

  std::map<int, std::vector<int>> grouping;

  for (auto it = begin; it != end; ++it) {
    // Add detector ID to the list of group detectors. Detector ID is always
    // spectra index + 1
    grouping[*it].push_back(static_cast<int>(std::distance(begin, it)) + 1);
  }

  for (auto it = grouping.begin(); it != grouping.end(); ++it) {
    if (it->first != 0) // Skip 0 group
    {
      TableRow newRow = detectorGroupingTable->appendRow();
      newRow << it->second;
    }
  }

  return detectorGroupingTable;
}

/** Load in a single spectrum taken from a NeXus file
*  @param hist ::     The workspace index
*  @param i ::        The spectrum index
*  @param specNo ::   The spectrum number
*  @param nxload ::   A reference to the MuonNeXusReader object
*  @param lengthIn :: The number of elements in a spectrum
*  @param localWorkspace :: A pointer to the workspace in which the data will be
* stored
*/
void LoadMuonNexus1::loadData(size_t hist, specid_t &i, specid_t specNo, MuonNexusReader &nxload,
                              const int64_t lengthIn,
                              DataObjects::Workspace2D_sptr localWorkspace) {
  // Read in a spectrum
  // Put it into a vector, discarding the 1st entry, which is rubbish
  // But note that the last (overflow) bin is kept
  // For Nexus, not sure if above is the case, hence give all data for now
  MantidVec &Y = localWorkspace->dataY(hist);
  Y.assign(nxload.counts + i * lengthIn,
           nxload.counts + i * lengthIn + lengthIn);

  // Create and fill another vector for the errors, containing sqrt(count)
  MantidVec &E = localWorkspace->dataE(hist);
  typedef double (*uf)(double);
  uf dblSqrt = std::sqrt;
  std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
  // Populate the workspace. Loop starts from 1, hence i-1

  // Create and fill another vector for the X axis  
  float *timeChannels = new float[lengthIn+1]();
  nxload.getTimeChannels(timeChannels, static_cast<const int>(lengthIn+1));
  // Put the read in array into a vector (inside a shared pointer)
  boost::shared_ptr<MantidVec> timeChannelsVec(
    new MantidVec(timeChannels, timeChannels + lengthIn+1));

  localWorkspace->setX(hist, timeChannelsVec);
  localWorkspace->getSpectrum(hist)->setSpectrumNo(specNo);

  // Clean up
  delete[] timeChannels;

}

/**  Log the run details from the file
* @param localWorkspace :: The workspace details to use
*/
void
LoadMuonNexus1::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace) {
  API::Run &runDetails = localWorkspace->mutableRun();

  runDetails.addProperty("run_title", localWorkspace->getTitle(), true);

  int numSpectra = static_cast<int>(localWorkspace->getNumberHistograms());
  runDetails.addProperty("nspectra", numSpectra);

  NXRoot root(m_filename);
  try {
    std::string start_time = root.getString("run/start_time");
    runDetails.addProperty("run_start", start_time);
  } catch (std::runtime_error &) {
    g_log.warning("run/start_time is not available, run_start log not added.");
  }

  try {
    std::string stop_time = root.getString("run/stop_time");
    runDetails.addProperty("run_end", stop_time);
  } catch (std::runtime_error &) {
    g_log.warning("run/stop_time is not available, run_end log not added.");
  }

  try {
    std::string dur = root.getString("run/duration");
    runDetails.addProperty("dur", dur);
    runDetails.addProperty("durunits", 1); // 1 means second here
    runDetails.addProperty("dur_secs", dur);
  } catch (std::runtime_error &) {
    g_log.warning("run/duration is not available, dur log not added.");
  }

  // Get number of good frames
  NXEntry runInstrumentBeam = root.openEntry("run/instrument/beam");
  NXInfo infoGoodTotalFrames = runInstrumentBeam.getDataSetInfo("frames_good");
  if (infoGoodTotalFrames.stat != NX_ERROR) {
    int dum = root.getInt("run/instrument/beam/frames_good");
    runDetails.addProperty("goodfrm", dum);
  }

  // Get sample parameters
  NXEntry runSample = root.openEntry("run/sample");

  if (runSample.containsDataSet("temperature")) {
    float temperature = runSample.getFloat("temperature");
    runDetails.addProperty("sample_temp", static_cast<double>(temperature));
  }

  if (runSample.containsDataSet("magnetic_field")) {
    float magn_field = runSample.getFloat("magnetic_field");
    runDetails.addProperty("sample_magn_field",
                           static_cast<double>(magn_field));
  }
}

/// Run the LoadLog Child Algorithm
void LoadMuonNexus1::runLoadLog(DataObjects::Workspace2D_sptr localWorkspace) {
  IAlgorithm_sptr loadLog = createChildAlgorithm("LoadMuonLog");
  // Pass through the same input filename
  loadLog->setPropertyValue("Filename", m_filename);
  // Set the workspace property to be the same one filled above
  loadLog->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadLog->execute();
  } catch (std::runtime_error &) {
    g_log.error("Unable to successfully run LoadLog Child Algorithm");
  } catch (std::logic_error &) {
    g_log.error("Unable to successfully run LoadLog Child Algorithm");
  }

  if (!loadLog->isExecuted())
    g_log.error("Unable to successfully run LoadLog Child Algorithm");

  NXRoot root(m_filename);

  try {
    NXChar orientation = root.openNXChar("run/instrument/detector/orientation");
    // some files have no data there
    orientation.load();

    if (orientation[0] == 't') {
      Kernel::TimeSeriesProperty<double> *p =
          new Kernel::TimeSeriesProperty<double>("fromNexus");
      std::string start_time = root.getString("run/start_time");
      p->addValue(start_time, -90.0);
      localWorkspace->mutableRun().addLogData(p);
      setProperty("MainFieldDirection", "Transverse");
    } else {
      setProperty("MainFieldDirection", "Longitudinal");
    }
  } catch (...) {
    setProperty("MainFieldDirection", "Longitudinal");
  }
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMuonNexus1::confidence(Kernel::NexusDescriptor &descriptor) const {
  const auto &firstEntryNameType = descriptor.firstEntryNameType();
  const std::string root = "/" + firstEntryNameType.first;
  if (!descriptor.pathExists(root + "/analysis"))
    return 0;

  bool upperIDF(true);
  if (descriptor.pathExists(root + "/IDF_version"))
    upperIDF = true;
  else {
    if (descriptor.pathExists(root + "/idf_version"))
      upperIDF = false;
    else
      return 0;
  }

  try {
    std::string versionField = "idf_version";
    if (upperIDF)
      versionField = "IDF_version";

    auto &file = descriptor.data();
    file.openPath(root + "/" + versionField);
    int32_t version = 0;
    file.getData(&version);
    if (version != 1)
      return 0;

    file.openPath(root + "/analysis");
    std::string def = file.getStrData();
    if (def == "muonTD" || def == "pulsedTD") {
      // If all this succeeded then we'll assume this is an ISIS Muon NeXus file
      // version 2
      return 81;
    }
  } catch (...) {
  }
  return 0;
}

} // namespace DataHandling
} // namespace Mantid
