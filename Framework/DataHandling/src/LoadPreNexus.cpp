// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadPreNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadTOFRawNexus.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/SAX/InputSource.h>
#include <exception>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::size_t;
using std::string;
using std::vector;

namespace Mantid::DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadPreNexus)

static const string RUNINFO_PARAM("Filename");
static const string MAP_PARAM("MappingFilename");

/// @copydoc Mantid::API::IAlgorithm::name()
const std::string LoadPreNexus::name() const { return "LoadPreNexus"; }

/// @copydoc Mantid::API::IAlgorithm::version()
int LoadPreNexus::version() const { return 1; }

/// @copydoc Mantid::API::IAlgorithm::category()
const std::string LoadPreNexus::category() const { return "DataHandling\\PreNexus;Workflow\\DataHandling"; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadPreNexus::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filename = descriptor.filename();
  if (filename.size() > 12 ? (filename.compare(filename.size() - 12, 12, "_runinfo.xml") == 0) : false)
    return 80;
  else
    return 0;
}

/// @copydoc Mantid::API::Algorithm::init()
void LoadPreNexus::init() {
  // runfile to read in
  declareProperty(std::make_unique<FileProperty>(RUNINFO_PARAM, "", FileProperty::Load, "_runinfo.xml"),
                  "The name of the runinfo file to read, including its full or relative "
                  "path.");

  // copied (by hand) from LoadEventPreNexus2
  declareProperty(std::make_unique<FileProperty>(MAP_PARAM, "", FileProperty::OptionalLoad, ".dat"),
                  "File containing the pixel mapping (DAS pixels to pixel IDs) file "
                  "(typically INSTRUMENT_TS_YYYY_MM_DD.dat). The filename will be found "
                  "automatically if not specified.");
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(1);
  declareProperty("ChunkNumber", EMPTY_INT(), mustBePositive,
                  "If loading the file by sections ('chunks'), this is the "
                  "section number of this execution of the algorithm.");
  declareProperty("TotalChunks", EMPTY_INT(), mustBePositive,
                  "If loading the file by sections ('chunks'), this is the "
                  "total number of sections.");
  // TotalChunks is only meaningful if ChunkNumber is set
  // Would be nice to be able to restrict ChunkNumber to be <= TotalChunks at
  // validation
  setPropertySettings("TotalChunks", std::make_unique<VisibleWhenProperty>("ChunkNumber", IS_NOT_DEFAULT));
  std::vector<std::string> propOptions{"Auto", "Serial", "Parallel"};
  declareProperty("UseParallelProcessing", "Auto", std::make_shared<StringListValidator>(propOptions),
                  "Use multiple cores for loading the data?\n"
                  "  Auto: Use serial loading for small data sets, parallel "
                  "for large data sets.\n"
                  "  Serial: Use a single core.\n"
                  "  Parallel: Use all available cores.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("LoadMonitors", true, Direction::Input),
                  "Load the monitors from the file.");

  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/// @copydoc Mantid::API::Algorithm::exec()
void LoadPreNexus::exec() {
  string runinfo = this->getPropertyValue(RUNINFO_PARAM);
  string mapfile = this->getPropertyValue(MAP_PARAM);
  int chunkTotal = this->getProperty("TotalChunks");
  int chunkNumber = this->getProperty("ChunkNumber");
  if (isEmpty(chunkTotal) || isEmpty(chunkNumber)) {
    chunkNumber = EMPTY_INT();
    chunkTotal = EMPTY_INT();
  } else {
    if (chunkNumber > chunkTotal)
      throw std::out_of_range("ChunkNumber cannot be larger than TotalChunks");
  }
  string useParallel = this->getProperty("UseParallelProcessing");
  string wsname = this->getProperty("OutputWorkspace");
  bool loadmonitors = this->getProperty("LoadMonitors");

  // determine the event file names
  Progress prog(this, 0., .1, 1);
  vector<string> eventFilenames;
  string dataDir;
  this->parseRuninfo(runinfo, dataDir, eventFilenames);
  prog.doReport("parsed runinfo file");

  // do math for the progress bar
  size_t numFiles = eventFilenames.size() + 1; // extra 1 is nexus logs
  if (loadmonitors)
    numFiles++;
  double prog_start = .1;
  double prog_delta = (1. - prog_start) / static_cast<double>(numFiles);

  // load event files
  string temp_wsname;

  for (size_t i = 0; i < eventFilenames.size(); i++) {
    if (i == 0)
      temp_wsname = wsname;
    else
      temp_wsname = "__" + wsname + "_temp__";

    auto alg = createChildAlgorithm("LoadEventPreNexus", prog_start, prog_start + prog_delta);
    alg->setProperty("EventFilename", dataDir + eventFilenames[i]);
    alg->setProperty("MappingFilename", mapfile);
    alg->setProperty("ChunkNumber", chunkNumber);
    alg->setProperty("TotalChunks", chunkTotal);
    alg->setProperty("UseParallelProcessing", useParallel);
    alg->setPropertyValue("OutputWorkspace", temp_wsname);
    alg->executeAsChildAlg();
    prog_start += prog_delta;

    if (i == 0) {
      m_outputWorkspace = alg->getProperty("OutputWorkspace");
    } else {
      IEventWorkspace_sptr tempws = alg->getProperty("OutputWorkspace");
      // clean up properties before adding data
      Run &run = tempws->mutableRun();
      if (run.hasProperty("gd_prtn_chrg"))
        run.removeProperty("gd_prtn_chrg");
      if (run.hasProperty("proton_charge"))
        run.removeProperty("proton_charge");

      m_outputWorkspace += tempws;
    }
  }

  // load the logs
  this->runLoadNexusLogs(runinfo, dataDir, prog_start, prog_start + prog_delta);
  prog_start += prog_delta;

  // publish output workspace
  this->setProperty("OutputWorkspace", m_outputWorkspace);

  // load the monitor
  if (loadmonitors) {
    this->runLoadMonitors(prog_start, 1.);
  }
}

/**
 * Parse the runinfo file to find the names of the neutron event files.
 *
 * @param runinfo Runinfo file with full path.
 * @param dataDir Directory where the runinfo file lives.
 * @param eventFilenames vector of all possible event files. This is filled by
 *the algorithm.
 */
void LoadPreNexus::parseRuninfo(const string &runinfo, string &dataDir, vector<string> &eventFilenames) {
  eventFilenames.clear();

  // Create a Poco Path object for runinfo filename
  Poco::Path runinfoPath(runinfo, Poco::Path::PATH_GUESS);
  // Now lets get the directory
  Poco::Path dirPath(runinfoPath.parent());
  dataDir = dirPath.absolute().toString();
  g_log.debug() << "Data directory \"" << dataDir << "\"\n";

  std::ifstream in(runinfo.c_str());
  Poco::XML::InputSource src(in);

  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> pDoc = parser.parse(&src);

  Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ELEMENT);
  Poco::XML::Node *pNode = it.nextNode(); // root node
  while (pNode) {
    if (pNode->nodeName() == "RunInfo") // standard name for this type
    {
      pNode = pNode->firstChild();
      while (pNode) {
        if (pNode->nodeName() == "FileList") {
          pNode = pNode->firstChild();
          while (pNode) {
            if (pNode->nodeName() == "DataList") {
              pNode = pNode->firstChild();
              while (pNode) {
                if (pNode->nodeName() == "scattering") {
                  auto *element = static_cast<Poco::XML::Element *>(pNode);
                  eventFilenames.emplace_back(element->getAttribute("name"));
                }
                pNode = pNode->nextSibling();
              }
            } else // search for DataList
              pNode = pNode->nextSibling();
          }
        } else // search for FileList
          pNode = pNode->nextSibling();
      }
    } else // search for RunInfo
      pNode = pNode->nextSibling();
  }

  // report the results to the log
  if (eventFilenames.size() == 1) {
    g_log.debug() << "Found 1 event file: \"" << eventFilenames[0] << "\"\n";
  } else {
    g_log.debug() << "Found " << eventFilenames.size() << " event files:";
    for (const auto &eventFilename : eventFilenames) {
      g_log.debug() << "\"" << eventFilename << "\" ";
    }
    g_log.debug() << "\n";
  }
}

/**
 * Load logs from a nexus file onto the workspace.
 *
 * @param runinfo Runinfo file with full path.
 * @param dataDir Directory where the runinfo file lives.
 * @param prog_start Starting position for the progress bar.
 * @param prog_stop Ending position for the progress bar.
 */
void LoadPreNexus::runLoadNexusLogs(const string &runinfo, const string &dataDir, const double prog_start,
                                    const double prog_stop) {
  // determine the name of the file "inst_run"
  string shortName = runinfo.substr(runinfo.find_last_of("/\\") + 1);
  const string runInfoFileExt = "_runinfo.xml";
  if (const auto &pos = shortName.find(runInfoFileExt); pos != string::npos) {
    shortName.resize(pos);
  }
  g_log.debug() << "SHORTNAME = \"" << shortName << "\"\n";

  // put together a list of possible locations
  vector<string> possibilities;
  possibilities.emplace_back(dataDir + shortName + "_event.nxs"); // next to runinfo
  possibilities.emplace_back(dataDir + shortName + "_histo.nxs");
  possibilities.emplace_back(dataDir + shortName + ".nxs");
  possibilities.emplace_back(dataDir + "../NeXus/" + shortName + "_event.nxs"); // in NeXus directory
  possibilities.emplace_back(dataDir + "../NeXus/" + shortName + "_histo.nxs");
  possibilities.emplace_back(dataDir + "../NeXus/" + shortName + ".nxs");

  // run the algorithm
  bool loadedLogs = false;
  for (auto &possibility : possibilities) {
    if (Poco::File(possibility).exists()) {
      g_log.information() << "Loading logs from \"" << possibility << "\"\n";
      auto alg = createChildAlgorithm("LoadNexusLogs", prog_start, prog_stop);
      alg->setProperty("Workspace", m_outputWorkspace);
      alg->setProperty("Filename", possibility);
      alg->setProperty("OverwriteLogs", false);
      alg->executeAsChildAlg();
      loadedLogs = true;
      // Reload instrument so SNAP can use log values
      std::string entry_name = LoadTOFRawNexus::getEntryName(possibility);
      LoadEventNexus::runLoadInstrument(possibility, m_outputWorkspace, entry_name, this);
      break;
    }
  }

  if (!loadedLogs)
    g_log.notice() << "Did not find a nexus file to load logs from\n";
}

/**
 * Load the monitor files.
 *
 * @param prog_start Starting position for the progress bar.
 * @param prog_stop Ending position for the progress bar.
 */
void LoadPreNexus::runLoadMonitors(const double prog_start, const double prog_stop) {
  std::string mon_wsname = this->getProperty("OutputWorkspace");
  mon_wsname.append("_monitors");

  try {
    auto alg = createChildAlgorithm("LoadPreNexusMonitors", prog_start, prog_stop);
    alg->setPropertyValue("RunInfoFilename", this->getProperty(RUNINFO_PARAM));
    alg->setPropertyValue("OutputWorkspace", mon_wsname);
    alg->executeAsChildAlg();
    MatrixWorkspace_sptr mons = alg->getProperty("OutputWorkspace");
    this->declareProperty(std::make_unique<WorkspaceProperty<>>("MonitorWorkspace", mon_wsname, Direction::Output),
                          "Monitors from the Event NeXus file");
    this->setProperty("MonitorWorkspace", mons);
    // Add an internal pointer to monitor workspace in the 'main' workspace
    m_outputWorkspace->setMonitorWorkspace(mons);
  } catch (std::exception &e) {
    g_log.warning() << "Failed to load monitors: " << e.what() << "\n";
  }
}

} // namespace Mantid::DataHandling
