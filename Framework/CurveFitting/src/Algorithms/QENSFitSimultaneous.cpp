// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/QENSFitSimultaneous.h"
#include "MantidCurveFitting/Algorithms/QENSFitUtilities.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/StartsWithValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string/join.hpp>
#include <utility>

namespace {
Mantid::Kernel::Logger g_log("QENSFit");

using namespace Mantid::API;

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

MatrixWorkspace_sptr convertSpectrumAxis(const MatrixWorkspace_sptr &inputWorkspace) {
  auto convSpec = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  convSpec->setLogging(false);
  convSpec->setChild(true);
  convSpec->setProperty("InputWorkspace", inputWorkspace);
  convSpec->setProperty("OutputWorkspace", "__converted");
  convSpec->setProperty("Target", "ElasticQ");
  convSpec->setProperty("EMode", "Indirect");
  convSpec->execute();
  return convSpec->getProperty("OutputWorkspace");
}

MatrixWorkspace_sptr convertToElasticQ(const MatrixWorkspace_sptr &inputWorkspace, bool doThrow) {
  auto axis = inputWorkspace->getAxis(1);
  if (axis->isSpectra())
    return convertSpectrumAxis(inputWorkspace);
  else if (axis->isNumeric()) {
    if (axis->unit()->unitID() != "MomentumTransfer" && doThrow)
      throw std::runtime_error("Input must have axis values of Q");
    return inputWorkspace->clone();
  } else if (doThrow)
    throw std::runtime_error("Input workspace must have either spectra or numeric axis.");
  return inputWorkspace->clone();
}

struct ElasticQAppender {
  explicit ElasticQAppender(std::vector<MatrixWorkspace_sptr> &elasticInput)
      : m_elasticInput(elasticInput), m_converted() {}

  void operator()(const MatrixWorkspace_sptr &workspace, bool doThrow) {
    auto it = m_converted.find(workspace.get());
    if (it != m_converted.end())
      m_elasticInput.emplace_back(it->second);
    else {
      auto elasticQ = convertToElasticQ(workspace, doThrow);
      m_elasticInput.emplace_back(elasticQ);
      m_converted[workspace.get()] = elasticQ;
    }
  }

private:
  std::vector<MatrixWorkspace_sptr> &m_elasticInput;
  std::unordered_map<MatrixWorkspace *, MatrixWorkspace_sptr> m_converted;
};

std::vector<MatrixWorkspace_sptr> convertToElasticQ(const std::vector<MatrixWorkspace_sptr> &workspaces, bool doThrow) {
  std::vector<MatrixWorkspace_sptr> elasticInput;
  auto appendElasticQWorkspace = ElasticQAppender(elasticInput);
  appendElasticQWorkspace(workspaces[0], doThrow);

  for (auto i = 1u; i < workspaces.size(); ++i)
    appendElasticQWorkspace(workspaces[i], doThrow);
  return elasticInput;
}

std::string shortParameterName(const std::string &longName) {
  return longName.substr(longName.rfind('.') + 1, longName.size());
}

void setMultiDataProperties(const IAlgorithm &qensFit, IAlgorithm &fit, const MatrixWorkspace_sptr &workspace,
                            const std::string &suffix) {
  fit.setProperty("InputWorkspace" + suffix, workspace);

  int workspaceIndex = qensFit.getProperty("WorkspaceIndex" + suffix);
  fit.setProperty("WorkspaceIndex" + suffix, workspaceIndex);

  double startX = qensFit.getProperty("StartX" + suffix);
  double endX = qensFit.getProperty("EndX" + suffix);
  fit.setProperty("StartX" + suffix, startX);
  fit.setProperty("EndX" + suffix, endX);

  std::vector<double> exclude = qensFit.getProperty("Exclude" + suffix);
  fit.setProperty("Exclude" + suffix, exclude);
}

void setMultiDataProperties(const IAlgorithm &qensFit, IAlgorithm &fit,
                            const std::vector<MatrixWorkspace_sptr> &workspaces) {
  setMultiDataProperties(qensFit, fit, workspaces[0], "");

  for (auto i = 1u; i < workspaces.size(); ++i)
    setMultiDataProperties(qensFit, fit, workspaces[i], "_" + std::to_string(i));
}

IFunction_sptr convertToSingleDomain(IFunction_sptr function) {
  auto composite = std::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite && composite->getNumberDomains() > 1)
    return composite->getFunction(0);
  return function;
}

WorkspaceGroup_sptr makeGroup(const Workspace_sptr &workspace) {
  auto group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  if (group)
    return group;
  group = WorkspaceGroup_sptr(new WorkspaceGroup);
  group->addWorkspace(workspace);
  return group;
}

ITableWorkspace_sptr transposeFitTable(const ITableWorkspace_sptr &table, const IFunction &function,
                                       const std::string &yAxisType) {
  auto transposed = WorkspaceFactory::Instance().createTable();
  transposed->addColumn(yAxisType, "axis-1");

  auto parameters = function.getParameterNames();
  for (const auto &parameter : parameters) {
    transposed->addColumn("double", parameter);
    transposed->addColumn("double", parameter + "_Err");
  }

  auto numberOfParameters = parameters.size();
  for (std::size_t i = 0; i < table->rowCount() - 1; i += numberOfParameters) {
    auto row = transposed->appendRow().m_row;

    for (auto j = 0u; j < numberOfParameters; ++j) {
      auto column = 1 + j * 2;
      transposed->Double(row, column) = table->Double(i + j, 1);
      transposed->Double(row, column + 1) = table->Double(i + j, 2);
    }
  }
  return transposed;
}

std::string getAxisType(const MatrixWorkspace &workspace, std::size_t axisIndex) {
  return workspace.getAxis(axisIndex)->isNumeric() ? "double" : "str";
}

NumericAxis const *getNumericAxis(const MatrixWorkspace &workspace, std::size_t axisIndex) {
  return dynamic_cast<NumericAxis const *>(workspace.getAxis(axisIndex));
}

TextAxis const *getTextAxis(const MatrixWorkspace &workspace, std::size_t axisIndex) {
  return dynamic_cast<TextAxis const *>(workspace.getAxis(axisIndex));
}

std::vector<std::string> getUniqueWorkspaceNames(std::vector<std::string> &&workspaceNames) {
  std::set<std::string> uniqueNames(workspaceNames.begin(), workspaceNames.end());
  workspaceNames.assign(uniqueNames.begin(), uniqueNames.end());
  return std::move(workspaceNames);
}

auto getNumericAxisValueReader(std::size_t axisIndex) {
  return [axisIndex](const MatrixWorkspace &workspace, std::size_t index) {
    if (auto const axis = getNumericAxis(workspace, axisIndex))
      return axis->getValue(index);
    return 0.0;
  };
}

auto getTextAxisValueReader(std::size_t axisIndex) {
  return [axisIndex](const MatrixWorkspace &workspace, std::size_t index) {
    if (auto const axis = getTextAxis(workspace, axisIndex))
      return axis->label(index);
    return std::string();
  };
}

template <typename T, typename GetValue>
void addValuesToColumn(Column &column, const std::vector<MatrixWorkspace_sptr> &workspaces,
                       const Mantid::API::Algorithm &indexProperties, const GetValue &getValue) {
  const std::string prefix = "WorkspaceIndex";

  int index = indexProperties.getProperty(prefix);
  column.cell<T>(0) = getValue(*workspaces.front(), static_cast<std::size_t>(index));

  for (auto i = 1u; i < workspaces.size(); ++i) {
    const auto indexName = prefix + "_" + std::to_string(i);
    index = indexProperties.getProperty(indexName);
    column.cell<T>(i) = getValue(*workspaces[i], static_cast<std::size_t>(index));
  }
}

void addValuesToTableColumn(ITableWorkspace &table, const std::vector<MatrixWorkspace_sptr> &workspaces,
                            const Mantid::API::Algorithm &indexProperties, std::size_t columnIndex) {
  if (workspaces.empty())
    return;

  const auto column = table.getColumn(columnIndex);
  if (getNumericAxis(*workspaces.front(), 1))
    addValuesToColumn<double>(*column, workspaces, indexProperties, getNumericAxisValueReader(1));
  else if (getTextAxis(*workspaces.front(), 1))
    addValuesToColumn<std::string>(*column, workspaces, indexProperties, getTextAxisValueReader(1));
}

std::vector<std::size_t> createDatasetGrouping(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  std::vector<std::size_t> grouping;
  grouping.emplace_back(0);
  for (auto i = 1u; i < workspaces.size(); ++i) {
    if (workspaces[i] != workspaces[i - 1])
      grouping.emplace_back(i);
  }
  grouping.emplace_back(workspaces.size());
  return grouping;
}

WorkspaceGroup_sptr createGroup(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  WorkspaceGroup_sptr group(new WorkspaceGroup);
  for (auto &&workspace : workspaces)
    group->addWorkspace(workspace);
  return group;
}

WorkspaceGroup_sptr runParameterProcessingWithGrouping(IAlgorithm &processingAlgorithm,
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
DECLARE_ALGORITHM(QENSFitSimultaneous)

/// Algorithms name for identification. @see Algorithm::name
const std::string QENSFitSimultaneous::name() const { return "QENSFitSimultaneous"; }

/// Algorithm's version for identification. @see Algorithm::version
int QENSFitSimultaneous::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string QENSFitSimultaneous::category() const { return "Workflow\\MIDAS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string QENSFitSimultaneous::summary() const { return "Performs a simultaneous QENS fit"; }

/// Algorithm's see also for related algorithms. @see Algorithm::seeAlso
const std::vector<std::string> QENSFitSimultaneous::seeAlso() const {
  return {"ConvolutionFitSimultaneous", "IqtFitSimultaneous", "Fit"};
}

void QENSFitSimultaneous::initConcrete() {
  declareProperty("Ties", "", Kernel::Direction::Input);
  getPointerToProperty("Ties")->setDocumentation("Math expressions defining ties between parameters of "
                                                 "the fitting function.");
  declareProperty("Constraints", "", Kernel::Direction::Input);
  getPointerToProperty("Constraints")->setDocumentation("List of constraints");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("MaxIterations", 500, mustBePositive->clone(),
                  "Stop after this number of iterations if a good fit is not found");

  std::vector<std::string> minimizerOptions = API::FuncMinimizerFactory::Instance().getKeys();
  Kernel::IValidator_sptr minimizerValidator = std::make_shared<Kernel::StartsWithValidator>(minimizerOptions);

  declareProperty("Minimizer", "Levenberg-Marquardt", minimizerValidator, "Minimizer to use for fitting.");
  declareProperty("CalcErrors", false,
                  "Set to true to calcuate errors when output isn't created "
                  "(default is false).");
  declareProperty("ExtractMembers", false,
                  "If true, then each member of the fit will be extracted"
                  ", into their own workspace. These workspaces will have a histogram"
                  " for each spectrum (Q-value) and will be grouped.",
                  Direction::Input);
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("ConvolveMembers", false),
                  "If true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");
  declareProperty("OutputCompositeMembers", false,
                  "If true and CreateOutput is true then the value of each "
                  "member of a Composite Function is also output.");

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

  declareProperty("OutputFitStatus", true,
                  "Flag to output fit status information, which consists of the fit "
                  "OutputStatus and the OutputChiSquared");

  std::vector<std::string> costFuncOptions = API::CostFunctionFactory::Instance().getKeys();
  // select only CostFuncFitting variety
  for (auto &costFuncOption : costFuncOptions) {
    auto costFunc = std::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(costFuncOption));
    if (!costFunc) {
      costFuncOption = "";
    }
  }
  Kernel::IValidator_sptr costFuncValidator = std::make_shared<Kernel::ListValidator<std::string>>(costFuncOptions);
  declareProperty("CostFunction", "Least squares", costFuncValidator,
                  "The cost function to be used for the fit, default is Least squares", Kernel::Direction::InOut);
}

void QENSFitSimultaneous::execConcrete() {
  const auto outputBaseName = getOutputBaseName();

  if (!outputBaseName.empty()) {
    if (getPropertyValue("OutputParameterWorkspace").empty())
      setProperty("OutputParameterWorkspace", outputBaseName + "_Parameters");

    if (getPropertyValue("OutputWorkspaceGroup").empty())
      setProperty("OutputWorkspaceGroup", outputBaseName + "_Workspaces");
  }

  const auto inputWorkspaces = getWorkspaces();
  const auto workspaces = convertInputToElasticQ(inputWorkspaces);
  const auto singleDomainFunction = convertToSingleDomain(getProperty("Function"));

  const auto fitResult = performFit(inputWorkspaces, outputBaseName);
  const auto yAxisType = getAxisType(*workspaces.front(), 1);
  auto transposedTable = transposeFitTable(fitResult.first, *singleDomainFunction, yAxisType);
  addValuesToTableColumn(*transposedTable, workspaces, *this, 0);
  const auto parameterWs = processParameterTable(transposedTable);
  const auto groupWs = makeGroup(fitResult.second);
  const auto resultWs = processIndirectFitParameters(parameterWs, createDatasetGrouping(workspaces));
  AnalysisDataService::Instance().addOrReplace(getPropertyValue("OutputWorkspace"), resultWs);

  if (containsMultipleData(workspaces)) {
    renameWorkspaces(groupWs, getWorkspaceIndices(), outputBaseName, "_Workspace", getWorkspaceNames());
    auto inputWorkspaceNames = getUniqueWorkspaceNames(getWorkspaceNames());
    renameWorkspaces(resultWs, std::vector<std::string>(inputWorkspaceNames.size(), ""), outputBaseName, "_Result",
                     inputWorkspaceNames);
  } else {
    renameWorkspaces(resultWs, std::vector<std::string>({""}), outputBaseName, "_Result");
  }

  copyLogs(resultWs, workspaces);

  const bool doExtractMembers = getProperty("ExtractMembers");
  if (doExtractMembers)
    extractMembers(groupWs, workspaces, outputBaseName + "_Members");

  addAdditionalLogs(resultWs);
  copyLogs(std::dynamic_pointer_cast<MatrixWorkspace>(resultWs->getItem(0)), groupWs);

  setProperty("OutputWorkspace", resultWs);
  setProperty("OutputParameterWorkspace", parameterWs);
  setProperty("OutputWorkspaceGroup", groupWs);
}

std::pair<API::ITableWorkspace_sptr, API::Workspace_sptr>
QENSFitSimultaneous::performFit(const std::vector<MatrixWorkspace_sptr> &workspaces, const std::string &output) {
  IFunction_sptr function = getProperty("Function");
  const bool convolveMembers = getProperty("ConvolveMembers");
  const bool outputCompositeMembers = getProperty("OutputCompositeMembers");
  const bool ignoreInvalidData = getProperty("IgnoreInvalidData");
  const bool calcErrors = getProperty("CalcErrors");

  auto fit = createChildAlgorithm("Fit", 0.05, 0.90, true);
  fit->setProperty("Function", function);
  setMultiDataProperties(*this, *fit, workspaces);
  fit->setProperty("IgnoreInvalidData", ignoreInvalidData);
  fit->setProperty("DomainType", getPropertyValue("DomainType"));
  fit->setProperty("EvaluationType", getPropertyValue("EvaluationType"));
  fit->setPropertyValue("PeakRadius", getPropertyValue("PeakRadius"));
  fit->setProperty("Ties", getPropertyValue("Ties"));
  fit->setProperty("Constraints", getPropertyValue("Constraints"));
  fit->setPropertyValue("MaxIterations", getPropertyValue("MaxIterations"));
  fit->setProperty("Minimizer", getPropertyValue("Minimizer"));
  fit->setProperty("CostFunction", getPropertyValue("CostFunction"));
  fit->setProperty("CalcErrors", calcErrors);
  fit->setProperty("OutputCompositeMembers", outputCompositeMembers);
  fit->setProperty("ConvolveMembers", convolveMembers);
  fit->setProperty("CreateOutput", true);
  fit->setProperty("Output", output);
  fit->executeAsChildAlg();

  std::string status = fit->getProperty("OutputStatus");
  double chiSquared = fit->getProperty("OutputChi2overDoF");

  const bool outputFitStatus = getProperty("OutputFitStatus");
  if (outputFitStatus) {
    declareProperty("OutputStatus", "", Direction::Output);
    declareProperty("OutputChiSquared", 0.0, Direction::Output);
    setProperty("OutputStatus", status);
    setProperty("OutputChiSquared", chiSquared);
  }

  if (workspaces.size() == 1) {
    MatrixWorkspace_sptr outputWS = fit->getProperty("OutputWorkspace");
    return {fit->getProperty("OutputParameters"), outputWS};
  }

  WorkspaceGroup_sptr outputWS = fit->getProperty("OutputWorkspace");
  return {fit->getProperty("OutputParameters"), outputWS};
}

WorkspaceGroup_sptr QENSFitSimultaneous::processIndirectFitParameters(const ITableWorkspace_sptr &parameterWorkspace,
                                                                      const std::vector<std::size_t> &grouping) {
  std::string const xAxisUnit = getProperty("ResultXAxisUnit");
  auto pifp = createChildAlgorithm("ProcessIndirectFitParameters", 0.91, 0.95, false);
  pifp->setAlwaysStoreInADS(false);
  pifp->setProperty("InputWorkspace", parameterWorkspace);
  pifp->setProperty("ColumnX", "axis-1");
  pifp->setProperty("XAxisUnit", xAxisUnit);
  pifp->setProperty("ParameterNames", getFitParameterNames());
  pifp->setProperty("IncludeChiSquared", true);
  return runParameterProcessingWithGrouping(*pifp, grouping);
}

void QENSFitSimultaneous::copyLogs(const WorkspaceGroup_sptr &resultWorkspace,
                                   const std::vector<MatrixWorkspace_sptr> &workspaces) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  for (auto &&result : *resultWorkspace) {
    logCopier->setProperty("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(result));
    for (const auto &workspace : workspaces) {
      logCopier->setProperty("InputWorkspace", workspace);
      logCopier->executeAsChildAlg();
    }
  }
}

void QENSFitSimultaneous::copyLogs(const MatrixWorkspace_sptr &resultWorkspace,
                                   const WorkspaceGroup_sptr &resultGroup) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("InputWorkspace", resultWorkspace);

  for (const auto &workspace : *resultGroup) {
    logCopier->setProperty("OutputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(workspace));
    logCopier->executeAsChildAlg();
  }
}

void QENSFitSimultaneous::extractMembers(const WorkspaceGroup_sptr &resultGroupWs,
                                         const std::vector<MatrixWorkspace_sptr> &workspaces,
                                         const std::string &outputWsName) {
  std::vector<std::string> workspaceNames;
  for (auto i = 0u; i < workspaces.size(); ++i) {
    auto name = "__result_members_" + std::to_string(i);
    AnalysisDataService::Instance().addOrReplace(name, workspaces[i]);
    workspaceNames.emplace_back(name);
  }

  auto extractAlgorithm = extractMembersAlgorithm(resultGroupWs, outputWsName);
  extractAlgorithm->setProperty("InputWorkspaces", workspaceNames);
  extractAlgorithm->execute();

  for (const auto &workspaceName : workspaceNames)
    AnalysisDataService::Instance().remove(workspaceName);
}

void QENSFitSimultaneous::addAdditionalLogs(const API::WorkspaceGroup_sptr &group) {
  for (auto &&workspace : *group)
    addAdditionalLogs(workspace);
}

void QENSFitSimultaneous::addAdditionalLogs(const Workspace_sptr &resultWorkspace) {
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

  logAdder->setProperty("LogType", "Number");
  for (const auto &log : getAdditionalLogNumbers()) {
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add number logs");
  }
}

IAlgorithm_sptr QENSFitSimultaneous::extractMembersAlgorithm(const WorkspaceGroup_sptr &resultGroupWs,
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

std::vector<MatrixWorkspace_sptr> QENSFitSimultaneous::getWorkspaces() const {
  std::vector<MatrixWorkspace_sptr> workspaces;
  workspaces.reserve(m_workspacePropertyNames.size());
  for (const auto &propertyName : m_workspacePropertyNames) {
    Workspace_sptr workspace = getProperty(propertyName);
    workspaces.emplace_back(std::dynamic_pointer_cast<MatrixWorkspace>(workspace));
  }
  return workspaces;
}

std::vector<std::string> QENSFitSimultaneous::getWorkspaceIndices() const {
  std::vector<std::string> workspaceIndices;
  workspaceIndices.reserve(m_workspaceIndexPropertyNames.size());
  for (const auto &propertName : m_workspaceIndexPropertyNames) {
    std::string workspaceIndex = getPropertyValue(propertName);
    workspaceIndices.emplace_back(workspaceIndex);
  }
  return workspaceIndices;
}

std::vector<std::string> QENSFitSimultaneous::getWorkspaceNames() const {
  std::vector<std::string> workspaceNames;
  workspaceNames.reserve(m_workspacePropertyNames.size());
  for (const auto &propertName : m_workspacePropertyNames) {
    std::string workspaceName = getPropertyValue(propertName);
    workspaceNames.emplace_back(workspaceName);
  }
  return workspaceNames;
}

std::vector<MatrixWorkspace_sptr>
QENSFitSimultaneous::convertInputToElasticQ(const std::vector<MatrixWorkspace_sptr> &workspaces) const {
  return convertToElasticQ(workspaces, throwIfElasticQConversionFails());
}

std::string QENSFitSimultaneous::getOutputBaseName() const {
  const auto base = getPropertyValue("OutputWorkspace");
  auto position = base.rfind("_Result");
  if (position != std::string::npos)
    return base.substr(0, position);
  return base;
}

bool QENSFitSimultaneous::throwIfElasticQConversionFails() const { return false; }

bool QENSFitSimultaneous::isFitParameter(const std::string & /*unused*/) const { return true; }

std::vector<std::string> QENSFitSimultaneous::getFitParameterNames() const {
  const auto uniqueParameters = getUniqueParameterNames();
  std::vector<std::string> parameters;
  parameters.reserve(uniqueParameters.size());
  std::copy_if(uniqueParameters.begin(), uniqueParameters.end(), std::back_inserter(parameters),
               [&](const std::string &parameter) { return isFitParameter(parameter); });
  return parameters;
}

std::set<std::string> QENSFitSimultaneous::getUniqueParameterNames() const {
  IFunction_sptr function = getProperty("Function");
  std::set<std::string> nameSet;
  for (auto i = 0u; i < function->nParams(); ++i)
    nameSet.insert(shortParameterName(function->parameterName(i)));
  return nameSet;
}

std::map<std::string, std::string> QENSFitSimultaneous::getAdditionalLogStrings() const {
  const bool convolve = getProperty("ConvolveMembers");
  auto fitProgram = name();
  fitProgram = fitProgram.substr(0, fitProgram.rfind("Simultaneous"));

  auto logs = std::map<std::string, std::string>();
  logs["convolve_members"] = convolve ? "true" : "false";
  logs["fit_program"] = fitProgram;
  logs["fit_mode"] = "Simultaneous";
  return logs;
}

std::map<std::string, std::string> QENSFitSimultaneous::getAdditionalLogNumbers() const {
  return std::map<std::string, std::string>();
}

ITableWorkspace_sptr QENSFitSimultaneous::processParameterTable(ITableWorkspace_sptr parameterTable) {
  return parameterTable;
}

void QENSFitSimultaneous::renameWorkspaces(const API::WorkspaceGroup_sptr &outputGroup,
                                           std::vector<std::string> const &spectra, std::string const &outputBaseName,
                                           std::string const &endOfSuffix,
                                           std::vector<std::string> const &inputWorkspaceNames) {
  auto rename = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  const auto getNameSuffix = [&](std::size_t i) {
    std::string workspaceName = inputWorkspaceNames[i] + "_" + spectra[i] + endOfSuffix;
    return workspaceName;
  };
  return renameWorkspacesInQENSFit(this, rename, outputGroup, outputBaseName, getNameSuffix);
}

void QENSFitSimultaneous::renameWorkspaces(const API::WorkspaceGroup_sptr &outputGroup,
                                           std::vector<std::string> const &spectra, std::string const &outputBaseName,
                                           std::string const &endOfSuffix) {
  auto rename = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  auto getNameSuffix = [&](std::size_t i) { return spectra[i] + endOfSuffix; };
  return renameWorkspacesInQENSFit(this, rename, outputGroup, outputBaseName, getNameSuffix);
}

} // namespace Mantid::CurveFitting::Algorithms
