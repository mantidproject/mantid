#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::Workspace_sptr;
using Mantid::API::WorkspaceGroup;
using MantidQt::CustomInterfaces::Muon::LoadResult;
using MantidQt::CustomInterfaces::Muon::DeadTimesType;

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
    : m_instruments(instruments) {
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
 * Load data from the given files into a struct
 * @param files :: [input] List of files to load
 * @returns :: struct with loaded data
 */
LoadResult MuonAnalysisDataLoader::loadFiles(const QStringList &files) const {
  if (files.empty())
    throw std::invalid_argument("Supplied list of files is empty");

  LoadResult result;

  std::vector<Workspace_sptr> loadedWorkspaces;

  std::string instrName; // Instrument name all the run files should belong to

  // Go through all the files and try to load them
  for (const auto fileName : files) {
    std::string file = fileName.toStdString();

    // Set up load algorithm
    IAlgorithm_sptr load =
        AlgorithmManager::Instance().createUnmanaged("LoadMuonNexus");

    load->initialize();
    load->setChild(true);
    load->setLogging(false); // We'll take care of printing messages ourselves
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

} // namespace CustomInterfaces
} // namespace MantidQt
