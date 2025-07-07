// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/cast.hpp>
#include <boost/regex.hpp>

#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace {
using namespace Mantid::API;
using namespace Mantid::Kernel;

WorkspaceGroup_sptr getADSGroupWorkspace(const std::string &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(workspaceName);
}

MatrixWorkspace_sptr getADSMatrixWorkspace(const std::string &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
}

MatrixWorkspace_sptr convertSpectrumAxis(const MatrixWorkspace_sptr &inputWorkspace, const std::string &outputName) {
  auto convSpec = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  convSpec->setLogging(false);
  convSpec->setProperty("InputWorkspace", inputWorkspace);
  convSpec->setProperty("OutputWorkspace", outputName);
  convSpec->setProperty("Target", "ElasticQ");
  convSpec->setProperty("EMode", "Indirect");
  convSpec->execute();
  // Attempting to use getProperty("OutputWorkspace") on algorithm results in a
  // nullptr being returned
  return getADSMatrixWorkspace(outputName);
}

MatrixWorkspace_sptr cloneWorkspace(const MatrixWorkspace_sptr &inputWorkspace, const std::string &outputName) {
  Workspace_sptr workspace = inputWorkspace->clone();
  AnalysisDataService::Instance().addOrReplace(outputName, workspace);
  return std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

MatrixWorkspace_sptr convertToElasticQ(const MatrixWorkspace_sptr &inputWorkspace, const std::string &outputName,
                                       bool doThrow) {
  auto axis = inputWorkspace->getAxis(1);
  if (axis->isSpectra())
    return convertSpectrumAxis(inputWorkspace, outputName);
  else if (axis->isNumeric()) {
    if (axis->unit()->unitID() != "MomentumTransfer" && doThrow)
      throw std::runtime_error("Input must have axis values of Q");
    return cloneWorkspace(inputWorkspace, outputName);
  } else if (doThrow)
    throw std::runtime_error("Input workspace must have either spectra or numeric axis.");
  return cloneWorkspace(inputWorkspace, outputName);
}

struct ElasticQAppender {
  explicit ElasticQAppender(std::vector<MatrixWorkspace_sptr> &elasticInput)
      : m_elasticInput(elasticInput), m_converted() {}

  void operator()(const MatrixWorkspace_sptr &workspace, const std::string &outputBase, bool doThrow) {
    auto it = m_converted.find(workspace.get());
    if (it != m_converted.end())
      m_elasticInput.emplace_back(it->second);
    else {
      auto elasticQ = convertToElasticQ(workspace, outputBase + std::to_string(m_converted.size() + 1), doThrow);
      m_elasticInput.emplace_back(elasticQ);
      m_converted[workspace.get()] = elasticQ;
    }
  }

private:
  std::vector<MatrixWorkspace_sptr> &m_elasticInput;
  std::unordered_map<MatrixWorkspace *, MatrixWorkspace_sptr> m_converted;
};

std::vector<MatrixWorkspace_sptr> convertToElasticQ(const std::vector<MatrixWorkspace_sptr> &workspaces,
                                                    const std::string &outputBaseName, bool doThrow) {
  std::vector<MatrixWorkspace_sptr> elasticInput;
  auto appendElasticQWorkspace = ElasticQAppender(elasticInput);
  appendElasticQWorkspace(workspaces[0], outputBaseName, doThrow);

  for (auto i = 1u; i < workspaces.size(); ++i)
    appendElasticQWorkspace(workspaces[i], outputBaseName, doThrow);
  return elasticInput;
}

void extractFunctionNames(const CompositeFunction_sptr &composite, std::vector<std::string> &names) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    names.emplace_back(composite->getFunction(i)->name());
}

void extractFunctionNames(const IFunction_sptr &function, std::vector<std::string> &names) {
  auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    extractFunctionNames(composite, names);
  else
    names.emplace_back(function->name());
}

void extractConvolvedNames(const IFunction_sptr &function, std::vector<std::string> &names);

void extractConvolvedNames(const CompositeFunction_sptr &composite, std::vector<std::string> &names) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    extractConvolvedNames(composite->getFunction(i), names);
}

void extractConvolvedNames(const IFunction_sptr &function, std::vector<std::string> &names) {
  auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite) {
    if (composite->name() == "Convolution" && composite->nFunctions() > 1 &&
        composite->getFunction(0)->name() == "Resolution")
      extractFunctionNames(composite->getFunction(1), names);
    else
      extractConvolvedNames(composite, names);
  }
}

std::string constructInputString(const MatrixWorkspace_sptr &workspace, int specMin, int specMax) {
  std::ostringstream input;
  for (auto i = specMin; i < specMax + 1; ++i)
    input << workspace->getName() << ",i" << std::to_string(i) << ";";
  return input.str();
}

std::vector<std::string> extractWorkspaceNames(const std::string &input) {
  std::vector<std::string> v;
  boost::regex reg("([^,;]+),");
  std::for_each(boost::sregex_token_iterator(input.begin(), input.end(), reg, 1), boost::sregex_token_iterator(),
                [&v](const std::string &name) { v.emplace_back(name); });
  return v;
}

std::vector<std::string> getUniqueWorkspaceNames(const std::string &input) {
  auto workspaceNames = extractWorkspaceNames(input);
  std::set<std::string> uniqueNames(workspaceNames.begin(), workspaceNames.end());
  workspaceNames.assign(uniqueNames.begin(), uniqueNames.end());
  return workspaceNames;
}

std::vector<MatrixWorkspace_sptr> extractWorkspaces(const std::string &input) {
  const auto workspaceNames = extractWorkspaceNames(input);

  std::vector<MatrixWorkspace_sptr> workspaces;

  std::transform(workspaceNames.begin(), workspaceNames.end(), std::back_inserter(workspaces),
                 [](const auto &wsName) { return getADSMatrixWorkspace(wsName); });

  return workspaces;
}

std::vector<std::string> getSpectra(const std::string &input) {
  std::vector<std::string> spectra;
  boost::regex reg(",[i|sp](0|[1-9][0-9]*);?");
  std::copy(boost::sregex_token_iterator(input.begin(), input.end(), reg, 1), boost::sregex_token_iterator(),
            std::back_inserter(spectra));
  return spectra;
}

std::vector<std::string> getSuffices(const std::string &input) {
  std::vector<std::string> suffices;
  boost::regex reg(",[i|sp](0|[1-9][0-9]*);?");
  std::copy(boost::sregex_token_iterator(input.begin(), input.end(), reg, 0), boost::sregex_token_iterator(),
            std::back_inserter(suffices));
  return suffices;
}

std::string replaceWorkspaces(const std::string &input, const std::vector<MatrixWorkspace_sptr> &workspaces) {
  const auto suffices = getSuffices(input);
  std::stringstream newInput;
  for (auto i = 0u; i < workspaces.size(); ++i)
    newInput << workspaces[i]->getName() << suffices[i];
  return newInput.str();
}

void renameWorkspace(const Algorithm_sptr &renamer, const Workspace_sptr &workspace, const std::string &newName) {
  renamer->setProperty("InputWorkspace", workspace);
  renamer->setProperty("OutputWorkspace", newName);
  renamer->executeAsChildAlg();
}

void deleteTemporaries(const Algorithm_sptr &deleter, const std::string &base) {
  auto name = base + std::to_string(1);
  std::size_t i = 2;

  while (AnalysisDataService::Instance().doesExist(name)) {
    deleter->setProperty("Workspace", name);
    deleter->executeAsChildAlg();
    name = base + std::to_string(i++);
  }
}

std::string shortParameterName(const std::string &longName) {
  return longName.substr(longName.rfind('.') + 1, longName.size());
}

bool containsMultipleData(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  const auto &first = workspaces.front();
  return std::any_of(workspaces.cbegin(), workspaces.cend(),
                     [&first](const auto &workspace) { return workspace != first; });
}

template <typename F, typename Renamer>
void renameWorkspacesWith(const WorkspaceGroup_sptr &groupWorkspace, F const &getName, Renamer const &renamer) {
  std::unordered_map<std::string, std::size_t> nameCount;
  for (auto i = 0u; i < groupWorkspace->size(); ++i) {
    const auto name = getName(i);
    auto count = nameCount.find(name);

    if (count == nameCount.end()) {
      renamer(groupWorkspace->getItem(i), name);
      nameCount[name] = 1;
    } else
      renamer(groupWorkspace->getItem(i), name + "(" + std::to_string(++count->second) + ")");
  }
}

template <typename F>
void renameWorkspacesInQENSFit(Algorithm *qensFit, const Algorithm_sptr &renameAlgorithm,
                               WorkspaceGroup_sptr outputGroup, std::string const &outputBaseName,
                               const F &getNameSuffix) {
  Progress renamerProg(qensFit, 0.98, 1.0, outputGroup->size() + 1);
  renamerProg.report("Renaming group workspaces...");

  auto getName = [&](std::size_t i) { return outputBaseName + "_" + getNameSuffix(i); };

  auto renamer = [&](const Workspace_sptr &workspace, const std::string &name) {
    renameWorkspace(renameAlgorithm, workspace, name);
    renamerProg.report("Renamed workspace in group.");
  };
  renameWorkspacesWith(outputGroup, getName, renamer);
}

std::vector<std::size_t> createDatasetGrouping(const std::vector<MatrixWorkspace_sptr> &workspaces,
                                               std::size_t maximum) {
  std::vector<std::size_t> grouping;
  grouping.emplace_back(0);
  for (auto i = 1u; i < workspaces.size(); ++i) {
    if (workspaces[i] != workspaces[i - 1])
      grouping.emplace_back(i);
  }
  grouping.emplace_back(maximum);
  return grouping;
}

std::vector<std::size_t> createDatasetGrouping(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  return createDatasetGrouping(workspaces, workspaces.size());
}

WorkspaceGroup_sptr createGroup(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  WorkspaceGroup_sptr group(new WorkspaceGroup);
  for (auto &&workspace : workspaces)
    group->addWorkspace(workspace);
  return group;
}

WorkspaceGroup_sptr runParameterProcessingWithGrouping(Algorithm &processingAlgorithm,
                                                       const std::vector<std::size_t> &grouping) {
  std::vector<MatrixWorkspace_sptr> results;
  results.reserve(grouping.size() - 1);
  for (auto i = 0u; i < grouping.size() - 1; ++i) {
    processingAlgorithm.setProperty("StartRowIndex", static_cast<int>(grouping[i]));
    processingAlgorithm.setProperty("EndRowIndex", static_cast<int>(grouping[i + 1]) - 1);
    processingAlgorithm.setProperty("OutputWorkspace", "__Result");
    processingAlgorithm.execute();
    results.emplace_back(processingAlgorithm.getProperty("OutputWorkspace"));
  }
  return createGroup(results);
}

} // namespace

namespace Mantid::CurveFitting::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QENSFitSequential)

/// Algorithms name for identification. @see Algorithm::name
const std::string QENSFitSequential::name() const { return "QENSFitSequential"; }

/// Algorithm's version for identification. @see Algorithm::version
int QENSFitSequential::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string QENSFitSequential::category() const { return "Workflow\\MIDAS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string QENSFitSequential::summary() const { return "Performs a sequential fit for QENS data"; }

/// Algorithm's see also for related algorithms. @see Algorithm::seeAlso
const std::vector<std::string> QENSFitSequential::seeAlso() const {
  return {"ConvolutionFitSequential", "IqtFitSequential", "PlotPeakByLogValue"};
}

void QENSFitSequential::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "The input workspace for the fit. This property will be ignored if "
                  "'Input' is provided.");

  auto boundedV = std::make_shared<BoundedValidator<int>>();
  boundedV->setLower(0);

  declareProperty("SpecMin", 0, boundedV,
                  "The first spectrum to be used in "
                  "the fit. Spectra values can not be "
                  "negative. This property will be ignored if 'Input' is provided.",
                  Direction::Input);

  declareProperty("SpecMax", 0, boundedV,
                  "The final spectrum to be used in "
                  "the fit. Spectra values can not be "
                  "negative. This property will be ignored if 'Input' is provided.",
                  Direction::Input);

  declareProperty("Input", "",
                  "A list of sources of data to fit. \n"
                  "Sources can be either workspace names or file names followed optionally "
                  "by a list of spectra/workspace-indices \n"
                  "or values using the notation described in the description section of "
                  "the help page.");

  std::vector<std::string> unitOptions = UnitFactory::Instance().getKeys();
  unitOptions.emplace_back("");
  declareProperty("ResultXAxisUnit", "MomentumTransfer", std::make_shared<StringListValidator>(unitOptions),
                  "The unit to assign to the X Axis of the result workspace, "
                  "defaults to MomentumTransfer");

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "The output result workspace(s)");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputParameterWorkspace", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "The output parameter workspace");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspaceGroup", "", Direction::Output,
                                                                      PropertyMode::Optional),
                  "The output group workspace");

  declareProperty(std::make_unique<FunctionProperty>("Function", Direction::InOut),
                  "The fitting function, common for all workspaces in the input.");
  declareProperty("LogName", "axis-1",
                  "Name of the log value to plot the "
                  "parameters against. Default: use spectra "
                  "numbers.");
  declareProperty(std::make_unique<ArrayProperty<double>>("StartX"), "A value of x in, or on the low x "
                                                                     "boundary of, the first bin to "
                                                                     "include in\n"
                                                                     "the fit (default lowest value of x)");
  declareProperty(std::make_unique<ArrayProperty<double>>("EndX"), "A value in, or on the high x boundary "
                                                                   "of, the last bin the fitting range\n"
                                                                   "(default the highest value of x)");

  declareProperty("PassWSIndexToFunction", false,
                  "For each spectrum in Input pass its workspace index to all "
                  "functions that"
                  "have attribute WorkspaceIndex.");

  declareProperty("Minimizer", "Levenberg-Marquardt",
                  "Minimizer to use for fitting. Minimizers available are "
                  "'Levenberg-Marquardt', 'Simplex', 'FABADA',\n"
                  "'Conjugate gradient (Fletcher-Reeves imp.)', 'Conjugate "
                  "gradient (Polak-Ribiere imp.)' and 'BFGS'");

  const std::vector<std::string> costFuncOptions = CostFunctionFactory::Instance().getKeys();
  declareProperty("CostFunction", "Least squares", std::make_shared<StringListValidator>(costFuncOptions),
                  "Cost functions to use for fitting. Cost functions available "
                  "are 'Least squares' and 'Ignore positive peaks'",
                  Direction::InOut);

  declareProperty("MaxIterations", 500, boundedV,
                  "Stop after this number of iterations if a good fit is not "
                  "found");
  declareProperty("PeakRadius", 0,
                  "A value of the peak radius the peak functions should use. A "
                  "peak radius defines an interval on the x axis around the "
                  "centre of the peak where its values are calculated. Values "
                  "outside the interval are not calculated and assumed zeros."
                  "Numerically the radius is a whole number of peak widths "
                  "(FWHM) that fit into the interval on each side from the "
                  "centre. The default value of 0 means the whole x axis.");

  declareProperty("ExtractMembers", false,
                  "If true, then each member of the fit will be extracted"
                  ", into their own workspace. These workspaces will have a histogram"
                  " for each spectrum (Q-value) and will be grouped.",
                  Direction::Input);

  declareProperty("OutputCompositeMembers", false,
                  "If true and CreateOutput is true then the value of each "
                  "member of a Composite Function is also output.");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("ConvolveMembers", false),
                  "If true and OutputCompositeMembers is true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");

  const std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty("EvaluationType", "CentrePoint",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(evaluationTypes)),
                  "The way the function is evaluated: CentrePoint or Histogram.", Kernel::Direction::Input);

  const std::array<std::string, 2> fitTypes = {{"Sequential", "Individual"}};
  declareProperty("FitType", "Sequential", Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(fitTypes)),
                  "Defines the way of setting initial values. If set to Sequential every "
                  "next fit starts with parameters returned by the previous fit. If set to "
                  "Individual each fit starts with the same initial values defined in "
                  "the Function property. Allowed values: [Sequential, Individual]",
                  Kernel::Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<double>>("Exclude", ""),
                  "A list of pairs of real numbers, defining the regions to "
                  "exclude from the fit.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("ExcludeMultiple", ""),
                  "A list of Exclusion ranges, defining the regions to "
                  "exclude from the fit for each spectra. Must have the "
                  "same number of sets as the number of the spectra.");

  declareProperty("IgnoreInvalidData", false, "Flag to ignore infinities, NaNs and data with zero errors.");

  declareProperty("OutputFitStatus", false,
                  "Flag to output fit status information, which consists of the fit "
                  "OutputStatus and the OutputChiSquared");
}

std::map<std::string, std::string> QENSFitSequential::validateInputs() {
  std::map<std::string, std::string> errors;

  if (getPropertyValue("Input").empty()) {
    MatrixWorkspace_sptr workspace = getProperty("InputWorkspace");
    if (!workspace)
      errors["InputWorkspace"] = "No input string or input workspace was provided.";

    const int specMin = getProperty("SpecMin");
    const int specMax = getProperty("SpecMax");
    if (specMin > specMax)
      errors["SpecMin"] = "SpecMin must be less than or equal to SpecMax.";
  }
  const auto inputWorkspaces = getWorkspaces();
  const auto workspaces = convertInputToElasticQ(inputWorkspaces);
  const auto inputString = getInputString(workspaces);
  const auto spectra = getSpectra(inputString);
  const std::vector<double> startX = getProperty("StartX");
  const std::vector<double> endX = getProperty("EndX");
  if (startX.size() != endX.size()) {
    errors["StartX"] = "StartX have the same size as EndX";
  } else if (startX.size() != spectra.size() && startX.size() != 1) {
    errors["StartX"] = "StartX must be a single value or have a value for each spectra.";
  } else {
    for (size_t i = 0; i < startX.size(); i++) {
      if (startX[i] >= endX[i]) {
        errors["StartX"] = "StartX must be less than EndX";
      }
    }
  }
  return errors;
}

void QENSFitSequential::exec() {
  const auto outputBaseName = getOutputBaseName();

  if (getPropertyValue("OutputParameterWorkspace").empty())
    setProperty("OutputParameterWorkspace", outputBaseName + "_Parameters");

  if (getPropertyValue("OutputWorkspaceGroup").empty())
    setProperty("OutputWorkspaceGroup", outputBaseName + "_Workspaces");

  const auto inputWorkspaces = getWorkspaces();
  const auto workspaces = convertInputToElasticQ(inputWorkspaces);
  const auto inputString = getInputString(workspaces);
  const auto spectra = getSpectra(inputString);

  if (workspaces.empty() || spectra.empty() || (workspaces.size() > 1 && workspaces.size() != spectra.size()))
    throw std::invalid_argument("A malformed input string was provided.");

  const auto parameterWs = processParameterTable(performFit(inputString, outputBaseName));
  const auto resultWs = processIndirectFitParameters(parameterWs, getDatasetGrouping(workspaces));
  const auto groupWs = getADSGroupWorkspace(outputBaseName + "_Workspaces");
  AnalysisDataService::Instance().addOrReplace(getPropertyValue("OutputWorkspace"), resultWs);

  if (containsMultipleData(workspaces)) {
    const auto inputStringProp = getPropertyValue("Input");
    renameWorkspaces(groupWs, spectra, outputBaseName, "_Workspace", extractWorkspaceNames(inputStringProp));
    auto inputWorkspaceNames = getUniqueWorkspaceNames(inputStringProp);
    renameWorkspaces(resultWs, std::vector<std::string>(inputWorkspaceNames.size(), ""), outputBaseName, "_Result",
                     inputWorkspaceNames);
  } else {
    renameWorkspaces(groupWs, spectra, outputBaseName, "_Workspace");
    renameWorkspaces(resultWs, std::vector<std::string>({""}), outputBaseName, "_Result");
  }

  copyLogs(resultWs, workspaces);

  const bool doExtractMembers = getProperty("ExtractMembers");
  if (doExtractMembers)
    extractMembers(groupWs, workspaces, outputBaseName + "_Members");

  renameGroupWorkspace("__PDF_Workspace", spectra, outputBaseName, "_PDF");

  deleteTemporaryWorkspaces(outputBaseName);

  size_t itter = 0;
  for (auto results : resultWs->getAllItems()) {
    addAdditionalLogs(results);
    std::string resultWsName = results->getName();
    auto endLoc = resultWsName.find("__Result");
    std::string baseName = resultWsName.erase(endLoc);
    for (auto &workspace : groupWs->getAllItems()) {
      const std::string wsName = workspace->getName();
      if (wsName.find(baseName) != wsName.npos) {
        copyLogs(std::dynamic_pointer_cast<MatrixWorkspace>(results), groupWs);
        addFitRangeLogs(workspace, itter);
        itter++;
      }
    }
    addFitRangeLogs(results, itter - 1);
  }

  setProperty("OutputWorkspace", resultWs);
  setProperty("OutputParameterWorkspace", parameterWs);
  // Copy the group to prevent the ADS having two entries for one workspace
  auto outGroupWs = WorkspaceGroup_sptr(new WorkspaceGroup);
  for (auto item : groupWs->getAllItems()) {
    outGroupWs->addWorkspace(item);
  }
  setProperty("OutputWorkspaceGroup", outGroupWs);
}

std::map<std::string, std::string> QENSFitSequential::getAdditionalLogStrings() const {
  const bool convolve = getProperty("ConvolveMembers");
  auto fitProgram = name();
  fitProgram = fitProgram.substr(0, fitProgram.rfind("Sequential"));

  auto logs = std::map<std::string, std::string>();
  logs["sample_filename"] = getPropertyValue("InputWorkspace");
  logs["convolve_members"] = convolve ? "true" : "false";
  logs["fit_program"] = fitProgram;
  logs["fit_mode"] = "Sequential";
  return logs;
}

std::map<std::string, std::string> QENSFitSequential::getAdditionalLogNumbers() const {
  return std::map<std::string, std::string>();
}

void QENSFitSequential::addAdditionalLogs(const WorkspaceGroup_sptr &resultWorkspace) {
  for (const auto &workspace : *resultWorkspace)
    addAdditionalLogs(workspace);
}

void QENSFitSequential::addAdditionalLogs(const Workspace_sptr &resultWorkspace) {
  auto logAdder = createChildAlgorithm("AddSampleLog", -1.0, -1.0, false);
  logAdder->setProperty("Workspace", resultWorkspace);
  Progress logAdderProg(this, 0.99, 1.00, 6);
  logAdder->setProperty("LogType", "String");
  for (const auto &log : getAdditionalLogStrings()) {
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add text logs");
  }
  logAdderProg.report("Add number logs");
  for (const auto &log : getAdditionalLogNumbers()) {
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add number logs");
  }
}

void QENSFitSequential::addFitRangeLogs(const API::Workspace_sptr &resultWorkspace, size_t itter) {
  auto logAdder = createChildAlgorithm("AddSampleLog", -1.0, -1.0, false);
  logAdder->setProperty("Workspace", resultWorkspace);
  Progress logAdderProg(this, 0.99, 1.00, 6);
  logAdder->setProperty("LogType", "String");

  std::vector<double> startX = getProperty("StartX");
  logAdder->setProperty("LogName", "start_x");
  if (startX.size() == 1) {
    logAdder->setProperty("LogText", std::to_string(startX[0]));
  } else {
    logAdder->setProperty("LogText", std::to_string(startX[itter]));
  }
  logAdder->executeAsChildAlg();

  std::vector<double> endX = getProperty("EndX");
  logAdder->setProperty("LogName", "end_x");
  if (endX.size() == 1) {
    logAdder->setProperty("LogText", std::to_string(endX[0]));
  } else {
    logAdder->setProperty("LogText", std::to_string(endX[itter]));
  }
  logAdder->executeAsChildAlg();
}

std::string QENSFitSequential::getOutputBaseName() const {
  const auto base = getPropertyValue("OutputWorkspace");
  const auto position = base.rfind("_Result");
  if (position != std::string::npos)
    return base.substr(0, position);
  return base;
}

bool QENSFitSequential::throwIfElasticQConversionFails() const { return false; }

bool QENSFitSequential::isFitParameter(const std::string & /*unused*/) const { return true; }

std::vector<std::string> QENSFitSequential::getFitParameterNames() const {
  const auto uniqueParameters = getUniqueParameterNames();
  std::vector<std::string> parameters;
  parameters.reserve(uniqueParameters.size());
  std::copy_if(uniqueParameters.begin(), uniqueParameters.end(), std::back_inserter(parameters),
               [&](const std::string &parameter) { return isFitParameter(parameter); });
  return parameters;
}

std::set<std::string> QENSFitSequential::getUniqueParameterNames() const {
  IFunction_sptr function = getProperty("Function");
  std::set<std::string> nameSet;
  for (auto i = 0u; i < function->nParams(); ++i)
    nameSet.insert(shortParameterName(function->parameterName(i)));
  return nameSet;
}

void QENSFitSequential::deleteTemporaryWorkspaces(const std::string &outputBaseName) {
  auto deleter = createChildAlgorithm("DeleteWorkspace", -1.0, -1.0, false);
  deleter->setProperty("Workspace", outputBaseName + "_NormalisedCovarianceMatrices");
  deleter->executeAsChildAlg();

  deleter->setProperty("Workspace", outputBaseName + "_Parameters");
  deleter->executeAsChildAlg();

  deleteTemporaries(deleter, getTemporaryName());
}

std::vector<std::size_t>
QENSFitSequential::getDatasetGrouping(const std::vector<API::MatrixWorkspace_sptr> &workspaces) const {
  if (getPropertyValue("Input").empty()) {
    int maximum = getProperty("SpecMax");
    return createDatasetGrouping(workspaces, static_cast<std::size_t>(maximum + 1));
  }
  return createDatasetGrouping(workspaces);
}

WorkspaceGroup_sptr QENSFitSequential::processIndirectFitParameters(const ITableWorkspace_sptr &parameterWorkspace,
                                                                    const std::vector<std::size_t> &grouping) {
  std::string const columnX = getProperty("LogName");
  std::string const xAxisUnit = getProperty("ResultXAxisUnit");
  auto pifp = createChildAlgorithm("ProcessIndirectFitParameters", 0.91, 0.95, false);
  pifp->setAlwaysStoreInADS(false);
  pifp->setProperty("InputWorkspace", parameterWorkspace);
  pifp->setProperty("ColumnX", columnX);
  pifp->setProperty("XAxisUnit", xAxisUnit);
  pifp->setProperty("ParameterNames", getFitParameterNames());
  pifp->setProperty("IncludeChiSquared", true);
  return runParameterProcessingWithGrouping(*pifp, grouping);
}

ITableWorkspace_sptr QENSFitSequential::processParameterTable(ITableWorkspace_sptr parameterTable) {
  return parameterTable;
}

void QENSFitSequential::renameWorkspaces(WorkspaceGroup_sptr outputGroup, std::vector<std::string> const &spectra,
                                         std::string const &outputBaseName, std::string const &endOfSuffix,
                                         std::vector<std::string> const &inputWorkspaceNames) {
  auto rename = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  const auto getNameSuffix = [&](std::size_t i) {
    std::string workspaceName = inputWorkspaceNames[i] + "_" + spectra[i] + endOfSuffix;
    return workspaceName;
  };
  return renameWorkspacesInQENSFit(this, rename, std::move(outputGroup), outputBaseName, getNameSuffix);
}

void QENSFitSequential::renameWorkspaces(WorkspaceGroup_sptr outputGroup, std::vector<std::string> const &spectra,
                                         std::string const &outputBaseName, std::string const &endOfSuffix) {
  auto rename = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  auto getNameSuffix = [&](std::size_t i) { return spectra[i] + endOfSuffix; };
  return renameWorkspacesInQENSFit(this, rename, std::move(outputGroup), outputBaseName, getNameSuffix);
}

void QENSFitSequential::renameGroupWorkspace(std::string const &currentName, std::vector<std::string> const &spectra,
                                             std::string const &outputBaseName, std::string const &endOfSuffix) {
  if (AnalysisDataService::Instance().doesExist(currentName)) {
    auto const group = getADSGroupWorkspace(currentName);
    if (group)
      renameWorkspaces(group, spectra, outputBaseName, endOfSuffix);
  }
}

ITableWorkspace_sptr QENSFitSequential::performFit(const std::string &input, const std::string &output) {
  const std::vector<double> exclude = getProperty("Exclude");
  const std::vector<std::string> excludeMultiple = getProperty("ExcludeMultiple");
  const bool convolveMembers = getProperty("ConvolveMembers");
  const bool outputCompositeMembers = getProperty("OutputCompositeMembers");
  const bool passWsIndex = getProperty("PassWSIndexToFunction");
  const bool ignoreInvalidData = getProperty("IgnoreInvalidData");
  const bool outputFitStatus = getProperty("OutputFitStatus");
  IFunction_sptr inputFunction = getProperty("Function");

  // Run PlotPeaksByLogValue
  auto plotPeaks = createChildAlgorithm("PlotPeakByLogValue", 0.05, 0.90, true);
  plotPeaks->setProperty("Input", input);
  plotPeaks->setProperty("OutputWorkspace", output);
  plotPeaks->setProperty("Function", inputFunction);
  plotPeaks->setProperty("StartX", getPropertyValue("StartX"));
  plotPeaks->setProperty("EndX", getPropertyValue("EndX"));
  plotPeaks->setProperty("Exclude", exclude);
  plotPeaks->setProperty("ExcludeMultiple", excludeMultiple);
  plotPeaks->setProperty("IgnoreInvalidData", ignoreInvalidData);
  plotPeaks->setProperty("FitType", "Sequential");
  plotPeaks->setProperty("CreateOutput", true);
  plotPeaks->setProperty("OutputCompositeMembers", outputCompositeMembers);
  plotPeaks->setProperty("ConvolveMembers", convolveMembers);
  plotPeaks->setProperty("MaxIterations", getPropertyValue("MaxIterations"));
  plotPeaks->setProperty("Minimizer", getPropertyValue("Minimizer"));
  plotPeaks->setProperty("PassWSIndexToFunction", passWsIndex);
  plotPeaks->setProperty("PeakRadius", getPropertyValue("PeakRadius"));
  plotPeaks->setProperty("LogValue", getPropertyValue("LogName"));
  plotPeaks->setProperty("EvaluationType", getPropertyValue("EvaluationType"));
  plotPeaks->setProperty("FitType", getPropertyValue("FitType"));
  plotPeaks->setProperty("CostFunction", getPropertyValue("CostFunction"));
  plotPeaks->setProperty("OutputFitStatus", outputFitStatus);

  plotPeaks->executeAsChildAlg();

  if (outputFitStatus) {
    declareProperty(std::make_unique<ArrayProperty<std::string>>("OutputStatus", Direction::Output));
    declareProperty(std::make_unique<ArrayProperty<double>>("OutputChiSquared", Direction::Output));
    std::vector<std::string> outputStatus = plotPeaks->getProperty("OutputStatus");
    std::vector<double> outputChiSquared = plotPeaks->getProperty("OutputChiSquared");
    setProperty("OutputStatus", outputStatus);
    setProperty("OutputChiSquared", outputChiSquared);
  }

  return plotPeaks->getProperty("OutputWorkspace");
}

std::string QENSFitSequential::getInputString(const std::vector<MatrixWorkspace_sptr> &workspaces) const {
  const auto inputString = getPropertyValue("Input");
  if (!inputString.empty())
    return replaceWorkspaces(inputString, workspaces);
  return constructInputString(workspaces[0], getProperty("SpecMin"), getProperty("SpecMax"));
}

std::vector<MatrixWorkspace_sptr> QENSFitSequential::getWorkspaces() const {
  const auto inputString = getPropertyValue("Input");
  if (!inputString.empty()) {
    auto workspaceList = extractWorkspaces(inputString);
    return workspaceList;
  }
  // The static_cast should not be necessary but it is required to avoid a
  // "internal compiler error: segmentation fault" when compiling with gcc
  // and std=c++1z
  return std::vector<MatrixWorkspace_sptr>{static_cast<MatrixWorkspace_sptr>(getProperty("InputWorkspace"))};
}

std::vector<MatrixWorkspace_sptr>
QENSFitSequential::convertInputToElasticQ(const std::vector<MatrixWorkspace_sptr> &workspaces) const {
  return convertToElasticQ(workspaces, getTemporaryName(), throwIfElasticQConversionFails());
}

void QENSFitSequential::extractMembers(const WorkspaceGroup_sptr &resultGroupWs,
                                       const std::vector<API::MatrixWorkspace_sptr> &workspaces,
                                       const std::string &outputWsName) {
  std::vector<std::string> workspaceNames;
  std::transform(workspaces.begin(), workspaces.end(), std::back_inserter(workspaceNames),
                 [](const API::MatrixWorkspace_sptr &workspace) { return workspace->getName(); });

  auto extractAlgorithm = extractMembersAlgorithm(resultGroupWs, outputWsName);
  extractAlgorithm->setProperty("InputWorkspaces", workspaceNames);
  extractAlgorithm->execute();
}

void QENSFitSequential::copyLogs(const WorkspaceGroup_sptr &resultWorkspaces,
                                 std::vector<MatrixWorkspace_sptr> const &workspaces) {
  for (auto const &resultWorkspace : *resultWorkspaces)
    copyLogs(resultWorkspace, workspaces);
}

void QENSFitSequential::copyLogs(const Workspace_sptr &resultWorkspace,
                                 std::vector<MatrixWorkspace_sptr> const &workspaces) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("OutputWorkspace", resultWorkspace->getName());
  for (auto const &workspace : workspaces) {
    logCopier->setProperty("InputWorkspace", workspace);
    logCopier->executeAsChildAlg();
  }
}

void QENSFitSequential::copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const WorkspaceGroup_sptr &resultGroup) {
  for (auto const &workspace : *resultGroup)
    copyLogs(resultWorkspace, workspace);
}

void QENSFitSequential::copyLogs(const MatrixWorkspace_sptr &resultWorkspace, const Workspace_sptr &resultGroup) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("InputWorkspace", resultWorkspace);
  logCopier->setProperty("OutputWorkspace", resultGroup->getName());
  logCopier->executeAsChildAlg();
}

IAlgorithm_sptr QENSFitSequential::extractMembersAlgorithm(const WorkspaceGroup_sptr &resultGroupWs,
                                                           const std::string &outputWsName) const {
  const bool convolved = getProperty("ConvolveMembers");
  std::vector<std::string> convolvedMembers;
  IFunction_sptr function = getProperty("Function");

  if (convolved)
    extractConvolvedNames(function, convolvedMembers);

  auto extractMembersAlg = AlgorithmManager::Instance().create("ExtractQENSMembers");
  extractMembersAlg->setProperty("ResultWorkspace", resultGroupWs);
  extractMembersAlg->setProperty("OutputWorkspace", outputWsName);
  extractMembersAlg->setProperty("RenameConvolvedMembers", convolved);
  extractMembersAlg->setProperty("ConvolvedMembers", convolvedMembers);
  return extractMembersAlg;
}

std::string QENSFitSequential::getTemporaryName() const { return "__" + name() + "_ws"; }

} // namespace Mantid::CurveFitting::Algorithms
