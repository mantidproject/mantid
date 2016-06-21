#include "MantidQtCustomInterfaces/Muon/MuonAnalysisDataLoader.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisHelper.h"

using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::Workspace_sptr;
using MantidQt::CustomInterfaces::Muon::LoadResult;

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
    const Muon::DeadTimesType &deadTimesType, const QStringList &instruments,
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
    const Muon::DeadTimesType &deadTimesType,
    const std::string &deadTimesFile) {
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
      if (m_deadTimesType == Muon::DeadTimesType::FromFile) {
        load->setPropertyValue("DeadTimeTable", "__NotUsed");
      }
      load->setPropertyValue("DetectorGroupingTable", "__NotUsed");
    }

    load->execute();

    Workspace_sptr loadedWorkspace = load->getProperty("OutputWorkspace");

    if (fileName == files.first()) {
      instrName = getInstrumentName(loadedWorkspace);

      // Check that is a valid Muon instrument
      if (!m_instruments.contains(QString::fromStdString(instrName),
                                  Qt::CaseInsensitive)) {
        if (0 != instrName.compare("DEVA")) {
          // special case - no IDF but let it load anyway
          throw std::runtime_error("Instrument is not recognized: " +
                                   instrName);
        }
      }

      if (m_deadTimesType == Muon::DeadTimesType::FromFile) {
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

} // namespace CustomInterfaces
} // namespace MantidQt
