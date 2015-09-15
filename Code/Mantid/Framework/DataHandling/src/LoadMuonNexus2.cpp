//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadMuonNexus1.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidNexus/NexusClasses.h"
#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <cmath>
#include <numeric>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadMuonNexus2)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using namespace Mantid::NeXus;

/// Empty default constructor
LoadMuonNexus2::LoadMuonNexus2() : LoadMuonNexus() {}

/** Executes the right version of the muon nexus loader: versions 1 or 2.
*
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid
*values
*/
void LoadMuonNexus2::exec() {
  std::string filePath = getPropertyValue("Filename");
  LoadMuonNexus1 load1;
  load1.initialize();

  Kernel::NexusDescriptor descriptor(filePath);
  int confidence1 = load1.confidence(descriptor);
  int confidence2 = this->confidence(descriptor);

  // if none can load the file throw
  if (confidence1 < 80 && confidence2 < 80) {
    throw Kernel::Exception::FileError("Cannot open the file ", filePath);
  }

  if (confidence2 > confidence1) {
    // this loader
    doExec();
  } else {
    // version 1 loader
    IAlgorithm_sptr childAlg =
        createChildAlgorithm("LoadMuonNexus", 0, 1, true, 1);
    auto version1Loader = boost::dynamic_pointer_cast<API::Algorithm>(childAlg);
    version1Loader->copyPropertiesFrom(*this);
    version1Loader->executeAsChildAlg();
    this->copyPropertiesFrom(*version1Loader);
    API::Workspace_sptr outWS = version1Loader->getProperty("OutputWorkspace");
    setProperty("OutputWorkspace", outWS);
  }
}

/** Read in a muon nexus file of the version 2.
*
*  @throw Exception::FileError If the Nexus file cannot be found/opened
*  @throw std::invalid_argument If the optional properties are set to invalid
*values
*/
void LoadMuonNexus2::doExec() {
  // Create the root Nexus class
  NXRoot root(getPropertyValue("Filename"));

  int64_t iEntry = getProperty("EntryNumber");
  if (iEntry >= static_cast<int64_t>(root.groups().size())) {
    throw std::invalid_argument("EntryNumber is out of range");
  }

  // Open the data entry
  m_entry_name = root.groups()[iEntry].nxname;
  NXEntry entry = root.openEntry(m_entry_name);

  // Read in the instrument name from the Nexus file
  m_instrument_name = entry.getString("instrument/name");

  // Read the number of periods in this file
  if (entry.containsGroup("run")) {
    try {
      m_numberOfPeriods = entry.getInt("run/number_periods");
    } catch (::NeXus::Exception &) {
      // assume 1
      m_numberOfPeriods = 1;
    }
  } else {
    m_numberOfPeriods = 1;
  }

  // Need to extract the user-defined output workspace name
  Property *ws = getProperty("OutputWorkspace");
  std::string localWSName = ws->value();
  // If multiperiod, will need to hold the Instrument & Sample for copying
  boost::shared_ptr<Instrument> instrument;
  boost::shared_ptr<Sample> sample;

  std::string detectorName;
  // Only the first NXdata found
  for (unsigned int i = 0; i < entry.groups().size(); i++) {
    std::string className = entry.groups()[i].nxclass;
    if (className == "NXdata") {
      detectorName = entry.groups()[i].nxname;
      break;
    }
  }
  NXData dataGroup = entry.openNXData(detectorName);

  Mantid::NeXus::NXInt spectrum_index = dataGroup.openNXInt("spectrum_index");
  spectrum_index.load();
  m_numberOfSpectra = spectrum_index.dim0();

  // Call private method to validate the optional parameters, if set
  checkOptionalProperties();

  NXFloat raw_time = dataGroup.openNXFloat("raw_time");
  raw_time.load();
  int nBins = raw_time.dim0();
  std::vector<double> timeBins;
  timeBins.assign(raw_time(), raw_time() + nBins);
  timeBins.push_back(raw_time[nBins - 1] + raw_time[1] - raw_time[0]);

  // Calculate the size of a workspace, given its number of periods & spectra to
  // read
  int total_specs;
  if (m_interval || m_list) {
    total_specs = static_cast<int>(m_spec_list.size());
    if (m_interval) {
      total_specs += static_cast<int>((m_spec_max - m_spec_min + 1));
    } else {
      m_spec_max = -1; // to stop entering the min - max loop
    }
  } else {
    total_specs = static_cast<int>(m_numberOfSpectra);
    // for nexus return all spectra
    m_spec_min = 1;
    m_spec_max = m_numberOfSpectra; // was +1?
  }

  // Create the 2D workspace for the output
  DataObjects::Workspace2D_sptr localWorkspace =
      boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", total_specs,
                                              nBins + 1, nBins));
  // Set the unit on the workspace to muon time, for now in the form of a Label
  // Unit
  boost::shared_ptr<Kernel::Units::Label> lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          UnitFactory::Instance().create("Label"));
  lblUnit->setLabel("Time", Units::Symbol::Microsecond);
  localWorkspace->getAxis(0)->unit() = lblUnit;
  // Set y axis unit
  localWorkspace->setYUnit("Counts");

  // g_log.error()<<" number of perioids= "<<m_numberOfPeriods<<std::endl;
  WorkspaceGroup_sptr wsGrpSptr = WorkspaceGroup_sptr(new WorkspaceGroup);
  if (entry.containsDataSet("title")) {
    wsGrpSptr->setTitle(entry.getString("title"));
  }

  if (entry.containsDataSet("notes")) {
    wsGrpSptr->setComment(entry.getString("notes"));
  }

  if (m_numberOfPeriods > 1) {
    setProperty("OutputWorkspace",
                boost::dynamic_pointer_cast<Workspace>(wsGrpSptr));
  }

  // period_index is currently unused
  // Mantid::NeXus::NXInt period_index = dataGroup.openNXInt("period_index");
  // period_index.load();

  Mantid::NeXus::NXInt counts = dataGroup.openIntData();
  counts.load();

  NXInstrument instr = entry.openNXInstrument("instrument");

  if (instr.containsGroup("detector_fb")) {
    NXDetector detector = instr.openNXDetector("detector_fb");
    if (detector.containsDataSet("time_zero")) {
      double dum = detector.getFloat("time_zero");
      setProperty("TimeZero", dum);
    }
    if (detector.containsDataSet("first_good_time")) {
      double dum = detector.getFloat("first_good_time");
      setProperty("FirstGoodData", dum);
    }
  }

  API::Progress progress(this, 0., 1., m_numberOfPeriods * total_specs);
  // Loop over the number of periods in the Nexus file, putting each period in a
  // separate workspace
  for (int period = 0; period < m_numberOfPeriods; ++period) {

    if (period == 0) {
      // Only run the Child Algorithms once
      loadRunDetails(localWorkspace);
      runLoadInstrument(localWorkspace);
      loadLogs(localWorkspace, entry, period);
    } else // We are working on a higher period of a multiperiod raw file
    {
      localWorkspace = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          WorkspaceFactory::Instance().create(localWorkspace));
    }

    std::string outws("");
    if (m_numberOfPeriods > 1) {
      std::string outputWorkspace = "OutputWorkspace";
      std::stringstream suffix;
      suffix << (period + 1);
      outws = outputWorkspace + "_" + suffix.str();
      std::string WSName = localWSName + "_" + suffix.str();
      declareProperty(
          new WorkspaceProperty<Workspace>(outws, WSName, Direction::Output));
      if (wsGrpSptr)
        wsGrpSptr->addWorkspace(localWorkspace);
    }

    // create spectrum -> index correspondence
    std::map<int, int> index_spectrum;
    for (int i = 0; i < m_numberOfSpectra; ++i) {
      index_spectrum[spectrum_index[i]] = i;
    }

    int counter = 0;
    for (int spec = static_cast<int>(m_spec_min);
         spec <= static_cast<int>(m_spec_max); ++spec) {
      int i = index_spectrum[spec]; // if spec not found i is 0
      loadData(counts, timeBins, counter, period, i, localWorkspace);
      localWorkspace->getSpectrum(counter)->setSpectrumNo(spectrum_index[i]);
      counter++;
      progress.report();
    }

    // Read in the spectra in the optional list parameter, if set
    if (m_list) {
      for (unsigned int i = 0; i < m_spec_list.size(); ++i) {
        int spec = m_spec_list[i];
        int k = index_spectrum[spec]; // if spec not found k is 0
        loadData(counts, timeBins, counter, period, k, localWorkspace);
        localWorkspace->getSpectrum(counter)->setSpectrumNo(spectrum_index[k]);
        counter++;
        progress.report();
      }
    }
    // Just a sanity check
    assert(counter == total_specs);

    bool autogroup = getProperty("AutoGroup");

    if (autogroup) {
      g_log.warning(
          "Autogrouping is not implemented for muon NeXus version 2 files");
    }

    // Assign the result to the output workspace property
    if (m_numberOfPeriods > 1)
      setProperty(outws, boost::static_pointer_cast<Workspace>(localWorkspace));
    else {
      setProperty("OutputWorkspace",
                  boost::dynamic_pointer_cast<Workspace>(localWorkspace));
    }

  } // loop over periods
}

/** loadData
*  Load the counts data from an NXInt into a workspace
*/
void LoadMuonNexus2::loadData(const Mantid::NeXus::NXInt &counts,
                              const std::vector<double> &timeBins, int wsIndex,
                              int period, int spec,
                              API::MatrixWorkspace_sptr localWorkspace) {
  MantidVec &X = localWorkspace->dataX(wsIndex);
  MantidVec &Y = localWorkspace->dataY(wsIndex);
  MantidVec &E = localWorkspace->dataE(wsIndex);
  X.assign(timeBins.begin(), timeBins.end());

  int nBins = 0;
  int *data = NULL;

  if (counts.rank() == 3) {
    nBins = counts.dim2();
    data = &counts(period, spec, 0);
  } else if (counts.rank() == 2) {
    nBins = counts.dim1();
    data = &counts(spec, 0);
  } else {
    throw std::runtime_error("Data have unsupported dimansionality");
  }
  assert(nBins + 1 == static_cast<int>(timeBins.size()));

  Y.assign(data, data + nBins);
  typedef double (*uf)(double);
  uf dblSqrt = std::sqrt;
  std::transform(Y.begin(), Y.end(), E.begin(), dblSqrt);
}

/**  Load logs from Nexus file. Logs are expected to be in
*   /run/sample group of the file.
*   @param ws :: The workspace to load the logs to.
*   @param entry :: The Nexus entry
*   @param period :: The period of this workspace
*/
void LoadMuonNexus2::loadLogs(API::MatrixWorkspace_sptr ws, NXEntry &entry,
                              int period) {
  // Avoid compiler warning
  (void)period;

  std::string start_time = entry.getString("start_time");

  std::string sampleName = entry.getString("sample/name");
  NXMainClass runlogs = entry.openNXClass<NXMainClass>("sample");
  ws->mutableSample().setName(sampleName);

  for (std::vector<NXClassInfo>::const_iterator it = runlogs.groups().begin();
       it != runlogs.groups().end(); ++it) {
    NXLog nxLog = runlogs.openNXLog(it->nxname);
    Kernel::Property *logv = nxLog.createTimeSeries(start_time);
    if (!logv)
      continue;
    ws->mutableRun().addLogData(logv);
  }

  ws->setTitle(entry.getString("title"));

  if (entry.containsDataSet("notes")) {
    ws->setComment(entry.getString("notes"));
  }

  std::string run_num =
      boost::lexical_cast<std::string>(entry.getInt("run_number"));
  // The sample is left to delete the property
  ws->mutableRun().addLogData(
      new PropertyWithValue<std::string>("run_number", run_num));

  ws->populateInstrumentParameters();
}

/**  Log the run details from the file
* @param localWorkspace :: The workspace details to use
*/
void
LoadMuonNexus2::loadRunDetails(DataObjects::Workspace2D_sptr localWorkspace) {
  API::Run &runDetails = localWorkspace->mutableRun();

  runDetails.addProperty("run_title", localWorkspace->getTitle(), true);

  int numSpectra = static_cast<int>(localWorkspace->getNumberHistograms());
  runDetails.addProperty("nspectra", numSpectra);

  m_filename = getPropertyValue("Filename");
  NXRoot root(m_filename);
  NXEntry entry = root.openEntry(m_entry_name);

  std::string start_time = entry.getString("start_time");
  runDetails.addProperty("run_start", start_time);

  std::string stop_time = entry.getString("end_time");
  runDetails.addProperty("run_end", stop_time);

  if (entry.containsGroup("run")) {
    NXClass runRun = entry.openNXGroup("run");

    if (runRun.containsDataSet("good_total_frames")) {
      int dum = runRun.getInt("good_total_frames");
      runDetails.addProperty("goodfrm", dum);
    }

    if (runRun.containsDataSet("number_periods")) {
      int dum = runRun.getInt("number_periods");
      runDetails.addProperty("nperiods", dum);
    }
  }

  { // Duration taken to be stop_time minus stat_time
    DateAndTime start(start_time);
    DateAndTime end(stop_time);
    double duration_in_secs = DateAndTime::secondsFromDuration(end - start);
    runDetails.addProperty("dur_secs", duration_in_secs);
  }
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMuonNexus2::confidence(Kernel::NexusDescriptor &descriptor) const {
  const auto &firstEntryNameType = descriptor.firstEntryNameType();
  const std::string root = "/" + firstEntryNameType.first;
  if (!descriptor.pathExists(root + "/definition"))
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
    if (version != 2)
      return 0;

    file.openPath(root + "/definition");
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
