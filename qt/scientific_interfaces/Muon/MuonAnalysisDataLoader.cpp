#include "MuonAnalysisDataLoader.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MuonAnalysisHelper.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::Workspace_sptr;
using Mantid::API::WorkspaceGroup;
using MantidQt::CustomInterfaces::Muon::LoadResult;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;
using MantidQt::CustomInterfaces::Muon::AnalysisOptions;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MuonAnalysisDataLoader");
}

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param deadTimesType :: [input] Type of dead time correction
 * @param instruments :: [input] List of supported instruments
 * @param deadTimesFile :: [input] If "from disk", the name of the file (else
 * blank)
 */
MuonAnalysisDataLoader::MuonAnalysisDataLoader(
    const DeadTimesType &deadTimesType, const QStringList &instruments,
    const std::string &deadTimesFile)
    : m_instruments(instruments),
      m_cacheBlacklist("\\w*auto_\\w.tmp", Qt::CaseInsensitive,
                       QRegExp::RegExp2) {
  this->setDeadTimesType(deadTimesType, deadTimesFile);
}

/**
 * Set the dead time correction type
 * @param deadTimesType :: [input] Type of dead time correction
 * @param deadTimesFile :: [input] If "from disk", the name of the file (else
 * blank)
 */
void MuonAnalysisDataLoader::setDeadTimesType(
    const DeadTimesType &deadTimesType, const std::string &deadTimesFile) {
  m_deadTimesType = deadTimesType;
  m_deadTimesFile = deadTimesFile;
}

/**
 * Set the list of supported instruments
 * @param instruments :: [input] List of supported instruments
 */
void MuonAnalysisDataLoader::setSupportedInstruments(
    const QStringList &instruments) {
  m_instruments = instruments;
}

/**
 * Load data from the given files into a struct
 * @param files :: [input] List of files to load
 * @returns :: struct with loaded data
 */
LoadResult MuonAnalysisDataLoader::loadFiles(const QStringList &files) const {
  if (files.empty())
    throw std::invalid_argument("Supplied list of files is empty");

  // Convert list of files into a mangled map key
  const auto toString = [](QStringList qsl) {
    std::ostringstream oss;
    qsl.sort();
    for (const QString &qs : qsl) {
      oss << qs.toStdString() << ",";
    }
    return oss.str();
  };

  // Clean cache from stale files etc
  updateCache();
  // Check cache to see if we've loaded this set of files before
  const std::string fileString = toString(files);
  if (m_loadedDataCache.find(fileString) != m_loadedDataCache.end()) {
    g_log.information("Using cached workspace for file(s): " + fileString);
    return m_loadedDataCache[fileString];
  }

  LoadResult result;

  std::vector<Workspace_sptr> loadedWorkspaces;

  std::string instrName; // Instrument name all the run files should belong to

  // Go through all the files and try to load them
  for (const auto &fileName : files) {
    std::string file = fileName.toStdString();

    // Set up load algorithm
    IAlgorithm_sptr load =
        AlgorithmManager::Instance().createUnmanaged("LoadMuonNexus");

    load->initialize();
    load->setChild(true);
    load->setPropertyValue("Filename", file);

    // Just to pass validation
    load->setPropertyValue("OutputWorkspace", "__NotUsed");

    if (fileName == files.first()) {
      // These are only needed for the first file
      if (m_deadTimesType == DeadTimesType::FromFile) {
        load->setPropertyValue("DeadTimeTable", "__NotUsed");
      }
      load->setPropertyValue("DetectorGroupingTable", "__NotUsed");
    }

    load->execute();

    Workspace_sptr loadedWorkspace = load->getProperty("OutputWorkspace");

    if (fileName == files.first()) {
      instrName = getInstrumentName(loadedWorkspace);

      // Check that it is a valid Muon instrument
      if (!m_instruments.contains(QString::fromStdString(instrName),
                                  Qt::CaseInsensitive)) {
        if (0 != instrName.compare("DEVA")) {
          // special case - no IDF but let it load anyway
          throw std::runtime_error("Instrument is not recognized: " +
                                   instrName);
        }
      }

      if (m_deadTimesType == DeadTimesType::FromFile) {
        result.loadedDeadTimes = load->getProperty("DeadTimeTable");
      }
      result.loadedGrouping = load->getProperty("DetectorGroupingTable");
      result.mainFieldDirection =
          static_cast<std::string>(load->getProperty("MainFieldDirection"));
      result.timeZero = load->getProperty("TimeZero");
      result.firstGoodData = load->getProperty("FirstGoodData");
    } else {
      if (getInstrumentName(loadedWorkspace) != instrName)
        throw std::runtime_error(
            "All the files should be produced by the same instrument");
    }

    loadedWorkspaces.push_back(loadedWorkspace);
  }

  // Some of the ARGUS data files contain wrong information about the
  // instrument main field direction. It is always longitudinal.
  if (instrName == "ARGUS") {
    result.mainFieldDirection = "longitudinal";
  }

  if (loadedWorkspaces.size() == 1) {
    // If single workspace loaded - use it
    Workspace_sptr ws = loadedWorkspaces.front();
    result.loadedWorkspace = ws;
    result.label = MuonAnalysisHelper::getRunLabel(ws);
  } else {
    // If multiple workspaces loaded - sum them to get the one to work with
    try {
      result.loadedWorkspace =
          MuonAnalysisHelper::sumWorkspaces(loadedWorkspaces);
    } catch (std::exception &e) {
      std::ostringstream error;
      error << "Unable to sum workspaces together: " << e.what() << "\n";
      error << "Make sure they have equal dimensions and number of periods.";
      throw std::runtime_error(error.str());
    }
    result.label = MuonAnalysisHelper::getRunLabel(loadedWorkspaces);
  }

  // Cache the result if we should so we don't have to load it next time
  if (shouldBeCached(files)) {
    g_log.information("Caching loaded workspace for file(s): " + fileString);
    m_loadedDataCache[fileString] = result;
  }
  return result;
}

/**
 * Get instrument name from a workspace
 * @param workspace :: [input] Workspace to get instrument name from
 * @returns :: name of instrument (empty if failed to get it)
 */
std::string MuonAnalysisDataLoader::getInstrumentName(
    const Workspace_sptr workspace) const {
  if (workspace) {
    const auto period = MuonAnalysisHelper::firstPeriod(workspace);
    if (period) {
      const auto instrument = period->getInstrument();
      if (instrument) {
        return instrument->getName();
      }
    }
  }
  return "";
}

/**
 * Checks against an internal regex for files that match. If any files match
 * then none will be cached.
 * @param filenames A list of file paths
 * @return True if they should be cached on loading, false otherwise.
 */
bool MuonAnalysisDataLoader::shouldBeCached(
    const QStringList &filenames) const {
  for (const auto &filename : filenames) {
    if (m_cacheBlacklist.indexIn(filename) >= 0) {
      // match indicates not caching
      return false;
    }
  }
  return true;
}

/**
 * Correct loaded data for dead times (if present) and group
 * @param loadedData :: [input] Load result
 * @param grouping :: [input] Grouping to use
 * @returns :: Workspace containing processed data
 */
Workspace_sptr MuonAnalysisDataLoader::correctAndGroup(
    const Muon::LoadResult &loadedData,
    const Mantid::API::Grouping &grouping) const {
  ITableWorkspace_sptr deadTimes;
  try { // to get the dead time correction
    deadTimes = getDeadTimesTable(loadedData);
  } catch (const std::exception &e) {
    // If dead correction wasn't applied we can still continue, though should
    // make user aware of that
    g_log.warning() << "No dead time correction applied: " << e.what() << "\n";
  }

  // Now apply DTC, if used, and grouping
  Workspace_sptr correctedGroupedWS;
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("MuonProcess");
  alg->initialize();
  alg->setProperty("InputWorkspace", loadedData.loadedWorkspace);
  alg->setProperty("Mode", "CorrectAndGroup");
  if (deadTimes) {
    alg->setProperty("ApplyDeadTimeCorrection", true);
    alg->setProperty("DeadTimeTable", deadTimes);
  }
  alg->setProperty("LoadedTimeZero", loadedData.timeZero);
  alg->setProperty("DetectorGroupingTable", grouping.toTable());
  alg->setChild(true);
  alg->setPropertyValue("OutputWorkspace", "__NotUsed");
  alg->execute();
  correctedGroupedWS = alg->getProperty("OutputWorkspace");
  return correctedGroupedWS;
}
/**
 * Gets dead times table from loaded data
 * @param loadedData :: [input] Load result
 * @returns :: dead times table
 */
ITableWorkspace_sptr
MuonAnalysisDataLoader::getDeadTimesTable(const LoadResult &loadedData) const {
  // Dead time table which will be used
  Workspace_sptr deadTimes;
  ITableWorkspace_sptr deadTimesTable;

  if (m_deadTimesType != DeadTimesType::None) {
    if (m_deadTimesType == DeadTimesType::FromFile) {
      if (!loadedData.loadedDeadTimes) {
        throw std::runtime_error(
            "Data file doesn't appear to contain dead time values");
      }
      deadTimes = loadedData.loadedDeadTimes;
    } else if (m_deadTimesType == DeadTimesType::FromDisk) {
      deadTimes = loadDeadTimesFromFile(m_deadTimesFile);
    }
  }

  // Convert dead times into a table
  if (deadTimes) {
    if (auto table = boost::dynamic_pointer_cast<ITableWorkspace>(deadTimes)) {
      deadTimesTable = table;
    } else if (auto group =
                   boost::dynamic_pointer_cast<WorkspaceGroup>(deadTimes)) {
      deadTimesTable =
          boost::dynamic_pointer_cast<ITableWorkspace>(group->getItem(0));
    }
  }

  return deadTimesTable;
}

/**
 * Loads dead time table (group of tables) from the file.
 * @param filename :: File to load dead times from
 * @return Table (group of tables) with dead times
 */
Workspace_sptr MuonAnalysisDataLoader::loadDeadTimesFromFile(
    const std::string &filename) const {
  try {
    IAlgorithm_sptr loadDeadTimes =
        AlgorithmManager::Instance().create("LoadNexusProcessed");
    loadDeadTimes->setChild(true);
    loadDeadTimes->setLogging(false); // We'll take care of logging ourself
    loadDeadTimes->setPropertyValue(
        "Filename", filename.empty() ? m_deadTimesFile : filename);
    loadDeadTimes->setPropertyValue("OutputWorkspace", "__NotUsed");
    loadDeadTimes->execute();

    return loadDeadTimes->getProperty("OutputWorkspace");
  } catch (std::exception &e) {
    std::ostringstream errorMsg;
    errorMsg << "Unable to load dead times from the specified file: "
             << e.what();
    throw std::runtime_error(errorMsg.str());
  }
}

/**
 * Perform analysis on the given workspace using the parameters supplied
 * (using the MuonProcess algorithm)
 * @param inputWS :: [input] Workspace to analyse (previously grouped and
 * dead-time corrected)
 * @param options :: [input] Struct containing parameters for what sort of
 * analysis to do
 * @returns :: Workspace containing analysed data
 */
Workspace_sptr MuonAnalysisDataLoader::createAnalysisWorkspace(
    const Workspace_sptr inputWS, const AnalysisOptions &options) const {
  IAlgorithm_sptr alg =
      AlgorithmManager::Instance().createUnmanaged("MuonProcess");

  alg->initialize();

  // Set input workspace property
  auto inputGroup = boost::make_shared<WorkspaceGroup>();
  // If is a group, will need to handle periods
  if (auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS)) {
    for (int i = 0; i < group->getNumberOfEntries(); i++) {
      auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(i));
      inputGroup->addWorkspace(ws);
    }
    alg->setProperty("SummedPeriodSet", options.summedPeriods);
    alg->setProperty("SubtractedPeriodSet", options.subtractedPeriods);
  } else if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
    // Put this single WS into a group and set it as the input property
    inputGroup->addWorkspace(ws);
    alg->setProperty("SummedPeriodSet", "1");
  } else {
    throw std::runtime_error(
        "Cannot create analysis workspace: unsupported workspace type");
  }
  alg->setProperty("InputWorkspace", inputGroup);

  // Set the rest of the algorithm properties
  setProcessAlgorithmProperties(alg, options);

  // We don't want workspace in the ADS so far
  alg->setChild(true);
  alg->setPropertyValue("OutputWorkspace", "__NotUsed");
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

/**
 * Set algorithm properties according to the given options
 * @param alg :: [input, output] Algorithm to set properties to
 * @param options :: [input] Options to get properties from
 */
void MuonAnalysisDataLoader::setProcessAlgorithmProperties(
    IAlgorithm_sptr alg, const AnalysisOptions &options) const {
  alg->setProperty("Mode", "Analyse");
  alg->setProperty("TimeZero", options.timeZero);             // user input
  alg->setProperty("LoadedTimeZero", options.loadedTimeZero); // from file
  alg->setProperty("CropWorkspace", false);
  alg->setProperty("Xmin", options.timeLimits.first);
  double Xmax = options.timeLimits.second;
  if (Xmax != Mantid::EMPTY_DBL()) {
    alg->setProperty("Xmax", Xmax);
  }
  if (!options.rebinArgs.empty()) {
    alg->setProperty("RebinParams", options.rebinArgs);
  }
  if (options.wsName != "") {
	  alg->setProperty("WorkspaceName", options.wsName);
  }
  // ---- Analysis ----

  // Find index of a name in a collection
  const auto indexOf = [](const std::string &name,
                          const std::vector<std::string> &collection) {
    return std::distance(collection.begin(),
                         std::find(collection.begin(), collection.end(), name));
  };
  if (isContainedIn(options.groupPairName, options.grouping.groupNames)) {
    // Group
    std::string outputType;
    switch (options.plotType) {
    case Muon::PlotType::Counts:
    case Muon::PlotType::Logarithm:
      outputType = "GroupCounts";
      break;
    case Muon::PlotType::Asymmetry:
      outputType = "GroupAsymmetry";
      break;
    default:
      throw std::invalid_argument(
          "Cannot create analysis workspace: Unsupported plot type");
    }
    alg->setProperty("OutputType", outputType);

    const auto groupNum =
        indexOf(options.groupPairName, options.grouping.groupNames);
    alg->setProperty("GroupIndex", static_cast<int>(groupNum));
  } else if (isContainedIn(options.groupPairName, options.grouping.pairNames)) {
    // Pair
    if (options.plotType == Muon::PlotType::Asymmetry)
      alg->setProperty("OutputType", "PairAsymmetry");
    else
      throw std::invalid_argument("Cannot create analysis workspace: Pairs "
                                  "support asymmetry plot type only");

    const auto pairNum =
        indexOf(options.groupPairName, options.grouping.pairNames);
    alg->setProperty(
        "PairFirstIndex",
        static_cast<int>(options.grouping.pairs.at(pairNum).first));
    alg->setProperty(
        "PairSecondIndex",
        static_cast<int>(options.grouping.pairs.at(pairNum).second));
    alg->setProperty("Alpha", options.grouping.pairAlphas.at(pairNum));
  } else {
    throw std::invalid_argument("Cannot create analysis workspace: Group/pair "
                                "name not found in grouping");
  }
}

/**
 * Checks each entry in the loaded data cache.
 * If the loaded workspace has since been deleted, or it is a workspace group
 * whose members have been deleted, then remove the cache entry.
 */
void MuonAnalysisDataLoader::updateCache() const {
  std::vector<std::string> invalidKeys;
  for (const auto &entry : m_loadedDataCache) {
    const auto &ws = entry.second.loadedWorkspace;
    if (!ws) { // Workspace has been deleted
      invalidKeys.push_back(entry.first);
    } else if (const auto wsGroup =
                   boost::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
      if (wsGroup->size() == 0) { // Group has been cleared
        invalidKeys.push_back(entry.first);
      }
    }
  }
  // Remove the invalid cache entries
  for (const auto &key : invalidKeys) {
    g_log.information("Erasing invalid cached entry for file(s): " + key);
    m_loadedDataCache.erase(key);
  }
}

void MuonAnalysisDataLoader::clearCache() {
  if (!m_loadedDataCache.empty()) {
    m_loadedDataCache.clear();
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
