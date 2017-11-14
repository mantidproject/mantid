#include "EnggDiffFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/PropertyWithValue.h"

#include <algorithm>
#include <numeric>

using namespace Mantid;

namespace { // helpers

template <typename T> void insertInOrder(const T &item, std::vector<T> &vec) {
  vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
}

bool isDigit(const std::string &text) {
  return std::all_of(text.cbegin(), text.cend(), ::isdigit);
}

} // anonymous namespace

namespace MantidQt {
namespace CustomInterfaces {

void EnggDiffFittingModel::addWorkspace(const int runNumber, const size_t bank,
                                        const API::MatrixWorkspace_sptr ws) {
	addToRunMap(runNumber, bank, m_wsMap, ws);
}

std::string EnggDiffFittingModel::getWorkspaceFilename(const int runNumber,
	const size_t bank){
	if (bank < 1 || bank > m_wsFilenameMap.size()) {
		throw std::runtime_error("Tried to get filename for runNumber " +
			std::to_string(runNumber) + " and bank number "
			+ std::to_string(bank));
	}
	if (m_wsFilenameMap[bank - 1].find(runNumber) == m_wsFilenameMap[bank - 1].end()) {
		throw std::runtime_error("Tried to get filename for runNumber " +
			std::to_string(runNumber) + " and bank number "
			+ std::to_string(bank));
	}
	return m_wsFilenameMap[bank - 1][runNumber];
}

Mantid::API::ITableWorkspace_sptr EnggDiffFittingModel::getFitResults(
	const int runNumber, const size_t bank){
  return getFromRunMap(runNumber, bank, m_fitResults);
}

void EnggDiffFittingModel::setDifcTzero(const int runNumber, const size_t bank,
	                 const std::vector<GSASCalibrationParms> &calibParams){
	auto ws = getWorkspace(runNumber, bank);
	auto &run = ws->mutableRun();
	const std::string units = "none";

	if (calibParams.empty()) {
		run.addProperty<double>("difc", DEFAULT_DIFC, units, true);
		run.addProperty<double>("difa", DEFAULT_DIFA, units, true);
		run.addProperty<double>("tzero", DEFAULT_TZERO, units, true);
	} else {
		GSASCalibrationParms params(0, 0.0, 0.0, 0.0);
		for (const auto &paramSet : calibParams) {
			if (paramSet.bankid == bank) {
				params = paramSet;
				break;
			}
		}
		if (params.difc == 0) {
			params = calibParams.front();
		}

		run.addProperty<double>("difc", params.difc, units, true);
		run.addProperty<double>("difa", params.difa, units, false);
		run.addProperty<double>("tzero", params.tzero, units, false);
	}
}

void EnggDiffFittingModel::enggFitPeaks(const int runNumber, const size_t bank,
	                                       const std::string &expectedPeaks){
	const auto ws = getWorkspace(runNumber, bank);
	auto enggFitPeaksAlg = 
		Mantid::API::AlgorithmManager::Instance().create("EnggFitPeaks");

	try {
		enggFitPeaksAlg->initialize();
		enggFitPeaksAlg->setProperty("InputWorkspace", ws);
		if (!expectedPeaks.empty()) {
			enggFitPeaksAlg->setProperty("ExpectedPeaks", expectedPeaks);
		}
		enggFitPeaksAlg->setProperty("FittedPeaks", FIT_RESULTS_TABLE_NAME);
		enggFitPeaksAlg->execute();

		API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
		const auto fitResultsTable = ADS.retrieveWS<API::ITableWorkspace>(FIT_RESULTS_TABLE_NAME);
		addToRunMap(runNumber, bank, m_fitResults, fitResultsTable);
		m_fitResults[bank - 1][runNumber] = fitResultsTable;
	}
	catch (std::exception) {
		throw std::runtime_error(
			"Could not run the algorithm EnggFitPeaks successfully.");
	}
}

void EnggDiffFittingModel::saveDiffFittingAscii(const int runNumber, const size_t bank, const std::string &filename){
	const auto ws = getFitResults(runNumber, bank);
	auto saveAlg = Mantid::API::AlgorithmManager::Instance().create("SaveDiffFittingAscii");
	saveAlg->initialize();
	saveAlg->setProperty("InputWorkspace", ws);
	saveAlg->setProperty("RunNumber", runNumber);
	saveAlg->setProperty("Bank", bank);
	saveAlg->setProperty("OutMode", "AppendToExistingFile");
	saveAlg->setProperty("Filename", filename);
	saveAlg->execute();
}

API::MatrixWorkspace_sptr
EnggDiffFittingModel::getWorkspace(const int runNumber, const size_t bank) {
  return getFromRunMap(runNumber, bank, m_wsMap);
}

std::vector<int> EnggDiffFittingModel::getAllRunNumbers() const {
  std::vector<int> runNumbers;

  for (const auto &workspaces : m_wsMap) {
    for (const auto &kvPair : workspaces) {
      const auto runNumber = kvPair.first;
      if (std::find(runNumbers.begin(), runNumbers.end(), runNumber) ==
          runNumbers.end()) {
        insertInOrder(runNumber, runNumbers);
      }
    }
  }

  return runNumbers;
}

void EnggDiffFittingModel::loadWorkspaces(const std::string &filename) {
  auto loadAlg = API::AlgorithmManager::Instance().create("Load");
  loadAlg->initialize();

  loadAlg->setPropertyValue("Filename", filename);
  loadAlg->setPropertyValue("OutputWorkspace", FOCUSED_WS_NAME);
  loadAlg->execute();

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  if (filename.find(",") == std::string::npos) { // Only 1 run loaded
    const auto ws = ADS.retrieveWS<API::MatrixWorkspace>(FOCUSED_WS_NAME);
    addWorkspace(ws->getRunNumber(), guessBankID(ws), filename, ws);
  } else {
    const auto group_ws = ADS.retrieveWS<API::WorkspaceGroup>(FOCUSED_WS_NAME);
	std::vector<std::string> filenames;
	boost::split(filenames, filename, boost::is_any_of(","));

	for (int i = 0; i != group_ws->getNumberOfEntries(); ++i) {
		const auto ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
			group_ws->getItem(i));
		addWorkspace(ws->getRunNumber(), guessBankID(ws), filenames[i], ws);
	}
  }
}

std::vector<std::pair<int, size_t>>
EnggDiffFittingModel::getRunNumbersAndBanksIDs() {
  std::vector<std::pair<int, size_t>> pairs;

  const auto runNumbers = getAllRunNumbers();
  for (const auto runNumber : runNumbers) {
    for (size_t i = 0; i < m_wsMap.size(); ++i) {
      if (m_wsMap[i].find(runNumber) != m_wsMap[i].end()) {
        pairs.push_back(std::pair<int, size_t>(runNumber, i + 1));
      }
    }
  }
  return pairs;
}

size_t
EnggDiffFittingModel::guessBankID(API::MatrixWorkspace_const_sptr ws) const {
  if (ws->run().hasProperty("bankid")) {
    const auto log = dynamic_cast<Kernel::PropertyWithValue<int> *>(
        ws->run().getLogData("bankid"));
    return boost::lexical_cast<size_t>(log->value());
  }

  // couldn't get it from sample logs - try using the old naming convention
  auto name = ws->getName();
  std::vector<std::string> chunks;
  boost::split(chunks, name, boost::is_any_of("_"));
  bool isNum = isDigit(chunks.back());
  if (!chunks.empty() && isNum) {
    try {
      return boost::lexical_cast<size_t>(chunks.back());
    } catch (boost::exception &) {
      // If we get a bad cast or something goes wrong then
      // the file is probably not what we were expecting
      // so throw a runtime error
      throw std::runtime_error(
          "Failed to fit file: The data was not what is expected. "
          "Does the file contain a focused workspace?");
    }
  }

  throw std::runtime_error("Could not guess run number from input workspace. "
                           "Are you sure it has been focused correctly?");
}

template<typename T, size_t S>
void EnggDiffFittingModel::addToRunMap(const int runNumber, const size_t bank,
	RunMap<S, T> &map, const T itemToAdd) {
	map[bank - 1][runNumber] = itemToAdd;

}

template <typename T, size_t S>
T EnggDiffFittingModel::getFromRunMap(const int runNumber, const size_t bank,
                                      RunMap<S, T> map) {
  if (bank < 1 || bank > map.size()) {
    throw std::runtime_error("Tried to access invalid bank: " + std::to_string(bank));
  }
  if (map[bank - 1].find(runNumber) == map[bank - 1].end()) {
    throw std::runtime_error("Accessed invalid run number " + std::to_string(runNumber)
		+ " for bank " + std::to_string(bank));
  }
  return map[bank - 1][runNumber];
}

void EnggDiffFittingModel::addWorkspace(const int runNumber, const size_t bank, 
	const std::string &filename, API::MatrixWorkspace_sptr ws){
	addToRunMap(runNumber, bank, m_wsFilenameMap, filename);
	addWorkspace(runNumber, bank, ws);
}

const std::string EnggDiffFittingModel::FOCUSED_WS_NAME =
    "engggui_fitting_focused_ws";
const std::string EnggDiffFittingModel::FIT_RESULTS_TABLE_NAME =
    "engggui_fitting_fitpeaks_params";

const double EnggDiffFittingModel::DEFAULT_DIFA = 0.0;
const double EnggDiffFittingModel::DEFAULT_DIFC = 18400.0;
const double EnggDiffFittingModel::DEFAULT_TZERO = 4.0;

} // namespace CustomInterfaces
} // namespace MantidQT
