#include "EnggDiffFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
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
	addToRunMap(runNumber, bank, m_focusedWorkspaceMap, ws);
}

std::string EnggDiffFittingModel::getWorkspaceFilename(const int runNumber,
	const size_t bank){
	return getFromRunMap(runNumber, bank, m_wsFilenameMap);
}

Mantid::API::ITableWorkspace_sptr EnggDiffFittingModel::getFitResults(
	const int runNumber, const size_t bank){
  return getFromRunMap(runNumber, bank, m_fitParamsMap);
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

		API::AnalysisDataServiceImpl &ADS = 
			API::AnalysisDataService::Instance();
		const auto fitResultsTable = ADS.retrieveWS<API::ITableWorkspace>(
			FIT_RESULTS_TABLE_NAME);
		addToRunMap(runNumber, bank, m_fitParamsMap, fitResultsTable);
		m_fitParamsMap[bank - 1][runNumber] = fitResultsTable;
	}
	catch (std::exception) {
		throw std::runtime_error(
			"Could not run the algorithm EnggFitPeaks successfully.");
	}
}

void EnggDiffFittingModel::saveDiffFittingAscii(const int runNumber, 
	const size_t bank, const std::string &filename){
	const auto ws = getFitResults(runNumber, bank);
	auto saveAlg = Mantid::API::AlgorithmManager::Instance().create(
		"SaveDiffFittingAscii");
	saveAlg->initialize();
	saveAlg->setProperty("InputWorkspace", ws);
	saveAlg->setProperty("RunNumber", std::to_string(runNumber));
	saveAlg->setProperty("Bank", std::to_string(bank));
	saveAlg->setProperty("OutMode", "AppendToExistingFile");
	saveAlg->setProperty("Filename", filename);
	saveAlg->execute();
}

void EnggDiffFittingModel::createFittedPeaksWS(
	const int runNumber, const size_t bank) {
	const auto fitFunctionParams = getFitResults(runNumber, bank);
	const auto focusedWS = getWorkspace(runNumber, bank);

	const size_t numberOfPeaks = fitFunctionParams->rowCount();
	const std::string fittedPeaksWSName = "engggui_fitting_single_peaks";

	for (size_t i = 0; i < numberOfPeaks; ++i) {
		const auto functionDescription = createFunctionString(fitFunctionParams, i);
		double startX, endX;
		std::tie(startX, endX) = getStartAndEndXFromFitParams(fitFunctionParams, i);

		const std::string singlePeakWSName = "__engggui_fitting_single_peak_" + std::to_string(i);

		evaluateFunction(functionDescription, focusedWS, singlePeakWSName,
			startX, endX);
		
		cropWorkspace(singlePeakWSName, singlePeakWSName, 1, 1);
		
		rebinToFocusedWorkspace(singlePeakWSName, runNumber, bank, 
			singlePeakWSName);

		if (i == 0) {
			cloneWorkspace(focusedWS, fittedPeaksWSName);
			setDataToClonedWS(singlePeakWSName, fittedPeaksWSName);
		}
		else {
			appendSpectra(fittedPeaksWSName, singlePeakWSName);
		}
	}

	alignDetectors(getWorkspace(runNumber, bank));
	alignDetectors(fittedPeaksWSName);

	const auto &ADS = Mantid::API::AnalysisDataService::Instance();
	const auto fittedPeaksWS = ADS.retrieveWS<Mantid::API::MatrixWorkspace>(fittedPeaksWSName);
	addToRunMap(runNumber, bank, m_fittedPeaksMap, fittedPeaksWS);
}

void EnggDiffFittingModel::evaluateFunction(
	const std::string &function, const Mantid::API::MatrixWorkspace_sptr inputWS,
	const std::string &outputWSName, const double startX, const double endX) {

	auto evalFunctionAlg = Mantid::API::AlgorithmManager::Instance().create("EvaluateFunction");
	evalFunctionAlg->initialize();
	evalFunctionAlg->setProperty("Function", function);
	evalFunctionAlg->setProperty("InputWorkspace", inputWS);
	evalFunctionAlg->setProperty("OutputWorkspace", outputWSName);
	evalFunctionAlg->setProperty("StartX", startX);
	evalFunctionAlg->setProperty("EndX", endX);
	evalFunctionAlg->execute();
}

void EnggDiffFittingModel::cropWorkspace(
	const std::string &inputWSName,	const std::string &outputWSName,
	const int startWSIndex, const int endWSIndex) {
	auto cropWSAlg = Mantid::API::AlgorithmManager::Instance().create("CropWorkspace");
	cropWSAlg->initialize();
	cropWSAlg->setProperty("InputWorkspace", inputWSName);
	cropWSAlg->setProperty("OutputWorkspace", outputWSName);
	cropWSAlg->setProperty("StartWorkspaceIndex", startWSIndex);
	cropWSAlg->setProperty("EndWorkspaceIndex", endWSIndex);
	cropWSAlg->execute();
}

void EnggDiffFittingModel::rebinToFocusedWorkspace(
	const std::string &wsToRebinName, const int runNumberToMatch,
	const size_t bankToMatch, const std::string &outputWSName){
	auto rebinToWSAlg =
		Mantid::API::AlgorithmManager::Instance().create("RebinToWorkspace");
	
	rebinToWSAlg->initialize();
	rebinToWSAlg->setProperty("WorkspaceToRebin", wsToRebinName);

	const auto wsToMatch = getWorkspace(runNumberToMatch, bankToMatch);
	rebinToWSAlg->setProperty("WorkspaceToMatch", wsToMatch);
	rebinToWSAlg->setProperty("OutputWorkspace", outputWSName);
	rebinToWSAlg->execute();
}

void EnggDiffFittingModel::cloneWorkspace(
	const Mantid::API::MatrixWorkspace_sptr inputWorkspace, 
	const std::string & outputWSName){
	auto cloneWSAlg = Mantid::API::AlgorithmManager::Instance().create("CloneWorkspace");
	cloneWSAlg->initialize();
	cloneWSAlg->setProperty("InputWorkspace", inputWorkspace);
	cloneWSAlg->setProperty("OutputWorkspace", outputWSName);
	cloneWSAlg->execute();
}

void EnggDiffFittingModel::setDataToClonedWS(
	const std::string & wsToCopyName, const std::string & targetWSName){
	auto &ADS = Mantid::API::AnalysisDataService::Instance();
	auto wsToCopy = ADS.retrieveWS<Mantid::API::MatrixWorkspace>(wsToCopyName);
	auto currentClonedWS = ADS.retrieveWS<Mantid::API::MatrixWorkspace>(targetWSName);
	currentClonedWS->mutableY(0) = wsToCopy->y(0);
	currentClonedWS->mutableE(0) = wsToCopy->e(0);
}

void EnggDiffFittingModel::appendSpectra(const std::string & ws1Name, 
	const std::string & ws2Name){
	auto appendSpectraAlg = Mantid::API::AlgorithmManager::Instance().create("AppendSpectra");

	appendSpectraAlg->initialize();
	appendSpectraAlg->setProperty("InputWorkspace1", ws1Name);
	appendSpectraAlg->setProperty("InputWorkspace2", ws2Name);
	appendSpectraAlg->setProperty("OutputWorksace", ws1Name);
	appendSpectraAlg->execute();
}

std::tuple<double, double, double> EnggDiffFittingModel::getDifcDifaTzero(
	Mantid::API::MatrixWorkspace_const_sptr ws){
	const auto run = ws->run();

	const auto difc = run.getPropertyValueAsType<double>("difc");
	const auto difa = run.getPropertyValueAsType<double>("difa");
	const auto tzero = run.getPropertyValueAsType<double>("tzero");

	return std::tuple<double, double, double>(difc, difa, tzero);
}

Mantid::API::ITableWorkspace_sptr 
EnggDiffFittingModel::createCalibrationParamsTable(
	Mantid::API::MatrixWorkspace_const_sptr inputWS){
	double difc, difa, tzero;
	std::tie(difc, difa, tzero) = getDifcDifaTzero(inputWS);

	auto calibrationParamsTable = Mantid::API::WorkspaceFactory::Instance().createTable();

	calibrationParamsTable->addColumn("int", "detid");
	calibrationParamsTable->addColumn("double", "difc");
	calibrationParamsTable->addColumn("double", "difa");
	calibrationParamsTable->addColumn("double", "tzero");

	Mantid::API::TableRow row = calibrationParamsTable->appendRow();
	const auto &spectrum = inputWS->getSpectrum(0);

	Mantid::detid_t detID = *(spectrum.getDetectorIDs().cbegin());
	row << detID << difc << difa << tzero;
	return calibrationParamsTable;
}

void EnggDiffFittingModel::convertFromDistribution(
	Mantid::API::MatrixWorkspace_sptr inputWS){
	auto convertFromDistAlg = 
		Mantid::API::AlgorithmManager::Instance().create("ConvertFromDistribution");
	convertFromDistAlg->initialize();
	convertFromDistAlg->setProperty("Workspace", inputWS);
	convertFromDistAlg->execute();
}

void EnggDiffFittingModel::alignDetectors(const std::string & wsName){
	const auto &ADS = Mantid::API::AnalysisDataService::Instance();
	const auto inputWS = ADS.retrieveWS<Mantid::API::MatrixWorkspace>(wsName);
	alignDetectors(inputWS);
}

void EnggDiffFittingModel::alignDetectors(Mantid::API::MatrixWorkspace_sptr inputWS){
	const auto calibrationParamsTable = createCalibrationParamsTable(inputWS);

	if (inputWS->isDistribution()) {
		convertFromDistribution(inputWS);
	}

	auto alignDetAlg = Mantid::API::AlgorithmManager::Instance().create("AlignDetectors");
	alignDetAlg->initialize();
	alignDetAlg->setProperty("InputWorkspace", inputWS);
	alignDetAlg->setProperty("OutputWorkspace", inputWS);
	alignDetAlg->setProperty("CalibrationWorkspace", calibrationParamsTable);
	alignDetAlg->execute();
}

API::MatrixWorkspace_sptr
EnggDiffFittingModel::getWorkspace(const int runNumber, const size_t bank) {
  return getFromRunMap(runNumber, bank, m_focusedWorkspaceMap);
}

std::vector<int> EnggDiffFittingModel::getAllRunNumbers() const {
  std::vector<int> runNumbers;

  for (const auto &workspaces : m_focusedWorkspaceMap) {
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
    for (size_t i = 0; i < m_focusedWorkspaceMap.size(); ++i) {
      if (m_focusedWorkspaceMap[i].find(runNumber) != m_focusedWorkspaceMap[i].end()) {
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
    throw std::invalid_argument("Tried to access invalid bank: " + 
		std::to_string(bank));
  }
  if (map[bank - 1].find(runNumber) == map[bank - 1].end()) {
    throw std::invalid_argument("Tried to access invalid run number " + 
		std::to_string(runNumber) + " for bank " + std::to_string(bank));
  }
  return map[bank - 1][runNumber];
}

void EnggDiffFittingModel::addWorkspace(const int runNumber, const size_t bank, 
	const std::string &filename, API::MatrixWorkspace_sptr ws){
	addToRunMap(runNumber, bank, m_wsFilenameMap, filename);
	addWorkspace(runNumber, bank, ws);
}

std::string EnggDiffFittingModel::createFunctionString(
	const Mantid::API::ITableWorkspace_sptr fitFunctionParams, 
	const size_t row){
	const auto A0 = fitFunctionParams->cell<double>(row, size_t(1));
	const auto A1 = fitFunctionParams->cell<double>(row, size_t(3));
	const auto I = fitFunctionParams->cell<double>(row, size_t(13));
	const auto A = fitFunctionParams->cell<double>(row, size_t(7));
	const auto B = fitFunctionParams->cell<double>(row, size_t(9));
	const auto X0 = fitFunctionParams->cell<double>(row, size_t(5));
	const auto S = fitFunctionParams->cell<double>(row, size_t(11));

	const std::string function =
		"name=LinearBackground,A0=" + boost::lexical_cast<std::string>(A0) +
		",A1=" + boost::lexical_cast<std::string>(A1) +
		";name=BackToBackExponential,I=" + boost::lexical_cast<std::string>(I) +
		",A=" + boost::lexical_cast<std::string>(A) + ",B=" +
		boost::lexical_cast<std::string>(B) + ",X0=" +
		boost::lexical_cast<std::string>(X0) + ",S=" +
		boost::lexical_cast<std::string>(S);
	return function;
}

std::pair<double, double> EnggDiffFittingModel::getStartAndEndXFromFitParams(
	const Mantid::API::ITableWorkspace_sptr fitFunctionParams, 
	const size_t row){
	const auto X0 = fitFunctionParams->cell<double>(row, size_t(5));
	const auto S = fitFunctionParams->cell<double>(row, size_t(11));
	const double windowLeft = 9;
	const double windowRight = 12;

	const auto startX = X0 - (windowLeft * S);
	const auto endX = X0 + (windowRight * S);
	return std::pair<double, double>(startX, endX);
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
