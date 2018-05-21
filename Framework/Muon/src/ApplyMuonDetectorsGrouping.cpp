#include "MantidMuon/ApplyMuonDetectorsGrouping.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <string>
#include <vector>

const std::vector<std::string> g_plotTypes = { "Counts", "Asymmetry"  };

namespace {

	// Find if name is in group/pair collection
	static bool isContainedIn(const std::string &name,
		const std::vector<std::string> &collection) {
		return std::find(collection.begin(), collection.end(), name) !=
			collection.end();
	}
};

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace MuonAlgorithmHelper;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyMuonDetectorGrouping);

void ApplyMuonDetectorGrouping::init() {

  std::string emptyString("");

  declareProperty(Mantid::Kernel::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", emptyString, Direction::Input,
                      PropertyMode::Mandatory),
                  "Input workspace containing data from detectors which are to "
                  "be grouped.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspaceGroup", emptyString, Direction::InOut,
          PropertyMode::Mandatory),
      "Input workspace group, the output will be saved here.");

  declareProperty("groupName", emptyString, "The name of the group.",
                  Direction::Input);
  declareProperty("Grouping", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);

  declareProperty(
      "plotType", "Counts",
      boost::make_shared<Kernel::ListValidator<std::string>>(g_plotTypes),
      "The type of analysis to perform on the spectra.", Direction::Input);

  declareProperty("TimeMin", 0.0, "start time for the data in ms.",
                  Direction::Input);
  setPropertySettings("TimeMin",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "plotType", Kernel::IS_EQUAL_TO, "Asymmetry"));

  declareProperty("TimeMax", 30.0, "end time for the data in ms.",
                  Direction::Input);
  setPropertySettings("TimeMax",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "plotType", Kernel::IS_EQUAL_TO, "Asymmetry"));

  declareProperty("RebinArgs", emptyString, "Rebin arguments.",
                  Direction::Input);

  declareProperty("TimeZero", 0.0, "....", Direction::Input);
  declareProperty("TimeLoadZero", 0.0, "....", Direction::Input);

  declareProperty("SummedPeriods", std::to_string(1), "...", Direction::Input);
  declareProperty("SubtractedPeriods", emptyString, "...", Direction::Input);

  // Perform Group Associations.

  std::string workspaceGrp("Workspaces");
  setPropertyGroup("InputWorkspace", workspaceGrp);
  setPropertyGroup("InputWorkspaceGroup", workspaceGrp);

  std::string groupingGrp("Grouping Information");
  setPropertyGroup("groupName", groupingGrp);
  setPropertyGroup("Grouping", groupingGrp);

  std::string analysisGrp("Analysis");
  setPropertyGroup("plotType", analysisGrp);
  setPropertyGroup("TimeMin", analysisGrp);
  setPropertyGroup("TimeMax", analysisGrp);
}

void ApplyMuonDetectorGrouping::exec() {

  WorkspaceGroup_sptr groupedWS = getProperty("InputWorkspaceGroup");
  Workspace_sptr inputWS        = getProperty("InputWorkspace");
  WorkspaceGroup_sptr muonWS    = getUserInput(inputWS, groupedWS);

  MatrixWorkspace_sptr clipWS;
  clipWS = boost::dynamic_pointer_cast<MatrixWorkspace>(muonWS->getItem(0));
  clipXRangeToWorkspace(clipWS);

  auto ws    = createAnalysisWorkspace(inputWS, false);
  auto wsRaw = createAnalysisWorkspace(inputWS, true);

  const std::string wsName = getNewWorkspaceName();
  const std::string wsRawName = wsName + "_Raw";

  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  ads.addOrReplace(wsName, ws);
  ads.addOrReplace(wsRawName, wsRaw);

  std::vector<std::string> wsNames = {wsName, wsRawName};
  MuonAlgorithmHelper::groupWorkspaces(m_groupedWS_name, wsNames);

}


/*
* Generate the name of the new workspace
*/
const std::string ApplyMuonDetectorGrouping::getNewWorkspaceName() {

	Muon::DatasetParams params;
	// don't fill in instrument, runs, periods.
	params.label = m_groupedWS_name;
	params.itemType = Muon::ItemType::Group;
	params.itemName = m_groupName;
	params.plotType = m_plotType;
	params.version = 1;
	const std::string wsName = generateWorkspaceName(params);
	return wsName;
}


/*
* Store the input properties as private member variables
*/
WorkspaceGroup_sptr ApplyMuonDetectorGrouping::getUserInput(Workspace_sptr& inputWS, WorkspaceGroup_sptr& groupedWS) {

	// Cast input WS to a grouping
	auto muonWS = boost::make_shared<WorkspaceGroup>();
	if (auto test = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
		muonWS->addWorkspace(inputWS);
	}
	else {
		muonWS = boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS);
	}

	m_groupedWS_name = groupedWS->getName();
	m_wsName = inputWS->getName();
	m_groupName = this->getProperty("groupName");
	m_groups = this->getProperty("Grouping");
	m_rebinArgs = this->getProperty("rebinArgs");
	m_Xmin = this->getProperty("TimeMin");
	m_Xmax = this->getProperty("TimeMax");
	m_timeZero = this->getProperty("TimeZero");
	m_loadedTimeZero = this->getProperty("TimeLoadZero");
	m_summedPeriods = this->getProperty("SummedPeriods");
	m_subtractedPeriods = this->getProperty("SubtractedPeriods");

	std::string plotType;
	plotType = this->getProperty("plotType");
	if (plotType == "Counts") {
		m_plotType = Muon::PlotType::Counts;
	}
	else if (plotType == "Asymmetry") {
		m_plotType = Muon::PlotType::Asymmetry;
	}
	else {
		// default to Counts.
		m_plotType = Muon::PlotType::Counts;
	}

	return muonWS;
}

/**
* Clip Xmin/Xmax to the range in the input WS
*/
void ApplyMuonDetectorGrouping::clipXRangeToWorkspace(MatrixWorkspace_sptr& ws) {
	double dataXMin;
	double dataXMax;
	ws->getXMinMax(dataXMin, dataXMax);
	if (m_Xmin < dataXMin) {
		m_Xmin = dataXMin;
	}
	if (m_Xmax > dataXMax) {
		m_Xmax = dataXMax;
	}
}


/**
* Creates workspace, processing the data using the MuonProcess algorithm.
*/
Workspace_sptr ApplyMuonDetectorGrouping::createAnalysisWorkspace(Workspace_sptr& inputWS, bool isRaw) {

  // Store all the options for Muon Process
  Grouping grouping;
  grouping.description = "no description";
  grouping.groupNames.emplace_back(m_groupName);
  grouping.groups.emplace_back(m_groups);

  Muon::AnalysisOptions options(grouping);

  options.summedPeriods     = m_summedPeriods;
  options.subtractedPeriods = m_subtractedPeriods;
  options.timeZero          = m_timeZero;
  options.loadedTimeZero    = m_loadedTimeZero;
  options.timeLimits.first  = m_Xmin;
  options.timeLimits.second = m_Xmax;
  options.rebinArgs         = isRaw ? "" : m_rebinArgs;
  options.plotType          = m_plotType;
  options.groupPairName     = m_groupName;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("MuonProcess");
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
	}
	else if (auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS)) {
		// Put this single WS into a group and set it as the input property
		inputGroup->addWorkspace(ws);
		alg->setProperty("SummedPeriodSet", "1");
	}
	else {
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
* Set algorithm properties according to the given options. For use with MuonProcess.
*/
void ApplyMuonDetectorGrouping::setProcessAlgorithmProperties(IAlgorithm_sptr alg, const Muon::AnalysisOptions &options) const {

	alg->setProperty("Mode", "Combined");
	alg->setProperty("DetectorGroupingTable", options.grouping.toTable());
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
	}
	else {
		throw std::invalid_argument("Cannot create analysis workspace: Group "
			"name not found in grouping");
	}
}

/**
* Performs validation of inputs to the algorithm.
* - Checks the bounds on X axis are sensible
* - Checks that the workspaceGroup is named differently to the workspace with the data
* @returns Map of parameter names to errors
*/
std::map<std::string, std::string> ApplyMuonDetectorGrouping::validateInputs() {
	std::map<std::string, std::string> errors;

	const std::string propTime("TimeMin");
	double tmin = this->getProperty("TimeMin");
	double tmax = this->getProperty("TimeMax");
	if (tmin > tmax) {
		errors[propTime] = "TimeMin > TimeMax";
	}
	if (tmin < 0) {
		errors[propTime] = "Time values are negative.";
	}
	if (tmax < 0) {
		errors["TimeMax"] = "Time values are negative.";
	}
	
	WorkspaceGroup_sptr groupedWS = getProperty("InputWorkspaceGroup");
	Workspace_sptr inputWS = getProperty("InputWorkspace");
	if (groupedWS->getName() == inputWS->getName()) {
		errors["InputWorkspaceGroup"] = "The InputWorkspaceGroup should not have the same name as InputWorkspace.";
	};

	return errors;
}

// Allow WorkspaceGroup property to function correctly.
bool ApplyMuonDetectorGrouping::checkGroups() {
	return false;
}

} // namespace DataHandling
} // namespace Mantid