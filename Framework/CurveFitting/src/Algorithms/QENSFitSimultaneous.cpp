#include "MantidCurveFitting/Algorithms/QENSFitSimultaneous.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/StartsWithValidator.h"

#include <boost/algorithm/string/join.hpp>

namespace {
Mantid::Kernel::Logger g_log("QENSFit");

using namespace Mantid::API;

void extractFunctionNames(CompositeFunction_sptr composite,
                          std::vector<std::string> &names) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    names.emplace_back(composite->getFunction(i)->name());
}

void extractFunctionNames(IFunction_sptr function,
                          std::vector<std::string> &names) {
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    extractFunctionNames(composite, names);
  else
    names.emplace_back(function->name());
}

void extractConvolvedNames(IFunction_sptr function,
                           std::vector<std::string> &names);

void extractConvolvedNames(CompositeFunction_sptr composite,
                           std::vector<std::string> &names) {
  for (auto i = 0u; i < composite->nFunctions(); ++i)
    extractConvolvedNames(composite->getFunction(i), names);
}

void extractConvolvedNames(IFunction_sptr function,
                           std::vector<std::string> &names) {
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite) {
    if (composite->name() == "Convolution" && composite->nFunctions() > 1 &&
        composite->getFunction(0)->name() == "Resolution")
      extractFunctionNames(composite->getFunction(1), names);
    else
      extractConvolvedNames(composite, names);
  }
}

MatrixWorkspace_sptr convertSpectrumAxis(MatrixWorkspace_sptr inputWorkspace,
                                         const std::string &outputName) {
  auto convSpec = AlgorithmManager::Instance().create("ConvertSpectrumAxis");
  convSpec->setLogging(false);
  convSpec->setProperty("InputWorkspace", inputWorkspace);
  convSpec->setProperty("OutputWorkspace", outputName);
  convSpec->setProperty("Target", "ElasticQ");
  convSpec->setProperty("EMode", "Indirect");
  convSpec->execute();
  // Attempting to use getProperty("OutputWorkspace") on algorithm results in a
  // nullptr being returned
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      outputName);
}

MatrixWorkspace_sptr cloneWorkspace(MatrixWorkspace_sptr inputWorkspace,
                                    const std::string &outputName) {
  Workspace_sptr workspace = inputWorkspace->clone();
  AnalysisDataService::Instance().addOrReplace(outputName, workspace);
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

MatrixWorkspace_sptr convertToElasticQ(MatrixWorkspace_sptr inputWorkspace,
                                       const std::string &outputName,
                                       bool doThrow) {
  auto axis = inputWorkspace->getAxis(1);
  if (axis->isSpectra())
    return convertSpectrumAxis(inputWorkspace, outputName);
  else if (axis->isNumeric()) {
    if (axis->unit()->unitID() != "MomentumTransfer" && doThrow)
      throw std::runtime_error("Input must have axis values of Q");
    return cloneWorkspace(inputWorkspace, outputName);
  } else if (doThrow)
    throw std::runtime_error(
        "Input workspace must have either spectra or numeric axis.");
  return cloneWorkspace(inputWorkspace, outputName);
}

struct ElasticQAppender {
  explicit ElasticQAppender(std::vector<MatrixWorkspace_sptr> &elasticInput)
      : m_elasticInput(elasticInput), m_converted() {}

  void operator()(MatrixWorkspace_sptr workspace, const std::string &outputBase,
                  bool doThrow) {
    auto it = m_converted.find(workspace.get());
    if (it != m_converted.end())
      m_elasticInput.emplace_back(it->second);
    else {
      auto elasticQ = convertToElasticQ(
          workspace, outputBase + std::to_string(m_converted.size() + 1),
          doThrow);
      m_elasticInput.emplace_back(elasticQ);
      m_converted[workspace.get()] = elasticQ;
    }
  }

private:
  std::vector<MatrixWorkspace_sptr> &m_elasticInput;
  std::unordered_map<MatrixWorkspace *, MatrixWorkspace_sptr> m_converted;
};

std::vector<MatrixWorkspace_sptr>
convertToElasticQ(const std::vector<MatrixWorkspace_sptr> &workspaces,
                  const std::string &outputBaseName, bool doThrow) {
  std::vector<MatrixWorkspace_sptr> elasticInput;
  auto appendElasticQWorkspace = ElasticQAppender(elasticInput);
  appendElasticQWorkspace(workspaces[0], outputBaseName, doThrow);

  for (auto i = 1u; i < workspaces.size(); ++i)
    appendElasticQWorkspace(workspaces[i], outputBaseName, doThrow);
  return elasticInput;
}

std::string shortParameterName(const std::string &longName) {
  return longName.substr(longName.rfind('.') + 1, longName.size());
}

bool containsMultipleData(const std::vector<MatrixWorkspace_sptr> &workspaces) {
  const auto &first = workspaces.front();
  for (const auto &workspace : workspaces) {
    if (workspace != first)
      return true;
  }
  return false;
}

void deleteTemporaries(IAlgorithm_sptr deleter, const std::string &base) {
  auto name = base + std::to_string(1);
  std::size_t i = 2;

  while (AnalysisDataService::Instance().doesExist(name)) {
    deleter->setProperty("Workspace", name);
    deleter->executeAsChildAlg();
    name = base + std::to_string(i++);
  }
}

void renameWorkspace(IAlgorithm_sptr renamer, Workspace_sptr workspace,
                     const std::string &newName) {
  renamer->setProperty("InputWorkspace", workspace);
  renamer->setProperty("OutputWorkspace", newName);
  renamer->executeAsChildAlg();
}

template <typename F, typename Renamer>
void renameWorkspacesWith(WorkspaceGroup_sptr groupWorkspace, F const &getName,
                          Renamer const &renamer) {
  std::unordered_map<std::string, std::size_t> nameCount;
  for (auto i = 0u; i < groupWorkspace->size(); ++i) {
    const auto name = getName(i);
    auto count = nameCount.find(name);

    if (count == nameCount.end()) {
      renamer(groupWorkspace->getItem(i), name);
      nameCount[name] = 1;
    } else
      renamer(groupWorkspace->getItem(i),
              name + "(" + std::to_string(++count->second) + ")");
  }
}

template <typename F>
void renameWorkspacesInQENSFit(Algorithm *qensFit,
                               IAlgorithm_sptr renameAlgorithm,
                               WorkspaceGroup_sptr outputGroup,
                               ITableWorkspace_sptr parameterWorkspace,
                               const F &getNameSuffix) {
  const auto groupName = qensFit->getPropertyValue("OutputWorkspaceGroup");
  const auto parameterName =
      qensFit->getPropertyValue("OutputParameterWorkspace");
  auto outputBase = groupName.substr(0, groupName.rfind("_Workspaces"));

  Progress renamerProg(qensFit, 0.98, 1.0, outputGroup->size() + 1);
  renamerProg.report("Renaming group workspaces...");

  auto getName = [&](std::size_t i) {
    return outputBase + "_" + getNameSuffix(i);
  };

  auto renamer = [&](Workspace_sptr workspace, const std::string &name) {
    renameWorkspace(renameAlgorithm, workspace, name);
    renamerProg.report("Renamed workspace in group.");
  };
  renameWorkspacesWith(outputGroup, getName, renamer);

  if (outputGroup->getName() != groupName)
    renameWorkspace(renameAlgorithm, outputGroup, groupName);
  if (parameterWorkspace->getName() != parameterName)
    renameWorkspace(renameAlgorithm, parameterWorkspace, parameterName);
}

void setMultiDataProperties(const IAlgorithm &qensFit, IAlgorithm &fit,
                            MatrixWorkspace_sptr workspace,
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

void setMultiDataProperties(
    const IAlgorithm &qensFit, IAlgorithm &fit,
    const std::vector<MatrixWorkspace_sptr> &workspaces) {
  setMultiDataProperties(qensFit, fit, workspaces[0], "");

  for (auto i = 1u; i < workspaces.size(); ++i)
    setMultiDataProperties(qensFit, fit, workspaces[i],
                           "_" + std::to_string(i));
}
} // namespace

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QENSFitSimultaneous)

/// Algorithms name for identification. @see Algorithm::name
const std::string QENSFitSimultaneous::name() const { return "QENSFit"; }

/// Algorithm's version for identification. @see Algorithm::version
int QENSFitSimultaneous::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string QENSFitSimultaneous::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string QENSFitSimultaneous::summary() const {
  return "Performs a simultaneous QENS fit";
}

/// Algorithm's see also for related algorithms. @see Algorithm::seeAlso
const std::vector<std::string> QENSFitSimultaneous::seeAlso() const {
  return {"ConvolutionFitSimultaneous", "IqtFitSimultaneous", "Fit"};
}

void QENSFitSimultaneous::initConcrete() {
  declareProperty("Ties", "", Kernel::Direction::Input);
  getPointerToProperty("Ties")->setDocumentation(
      "Math expressions defining ties between parameters of "
      "the fitting function.");
  declareProperty("Constraints", "", Kernel::Direction::Input);
  getPointerToProperty("Constraints")->setDocumentation("List of constraints");
  auto mustBePositive = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty(
      "MaxIterations", 500, mustBePositive->clone(),
      "Stop after this number of iterations if a good fit is not found");
  declareProperty("OutputStatus", "", Kernel::Direction::Output);
  getPointerToProperty("OutputStatus")
      ->setDocumentation("Whether the fit was successful");
  declareProperty("OutputChi2overDoF", 0.0, "Returns the goodness of the fit",
                  Kernel::Direction::Output);

  std::vector<std::string> minimizerOptions =
      API::FuncMinimizerFactory::Instance().getKeys();
  Kernel::IValidator_sptr minimizerValidator =
      boost::make_shared<Kernel::StartsWithValidator>(minimizerOptions);

  declareProperty("Minimizer", "Levenberg-Marquardt", minimizerValidator,
                  "Minimizer to use for fitting.");

  std::vector<std::string> costFuncOptions =
      API::CostFunctionFactory::Instance().getKeys();
  // select only CostFuncFitting variety
  for (auto &costFuncOption : costFuncOptions) {
    auto costFunc = boost::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(costFuncOption));
    if (!costFunc) {
      costFuncOption = "";
    }
  }
  Kernel::IValidator_sptr costFuncValidator =
      boost::make_shared<Kernel::ListValidator<std::string>>(costFuncOptions);
  declareProperty(
      "CostFunction", "Least squares", costFuncValidator,
      "The cost function to be used for the fit, default is Least squares",
      Kernel::Direction::InOut);
  declareProperty("CalcErrors", false,
                  "Set to true to calcuate errors when output isn't created "
                  "(default is false).");
  declareProperty(
      "ExtractMembers", false,
      "If true, then each member of the fit will be extracted"
      ", into their own workspace. These workspaces will have a histogram"
      " for each spectrum (Q-value) and will be grouped.",
      Direction::Input);
  declareProperty(Kernel::make_unique<Kernel::PropertyWithValue<bool>>(
                      "ConvolveMembers", false),
                  "If true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The output result workspace");
  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputParameterWorkspace", "", Direction::Output,
                      PropertyMode::Optional),
                  "The output parameter workspace");
  declareProperty(make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspaceGroup", "", Direction::Output,
                      PropertyMode::Optional),
                  "The output group workspace");
}

void QENSFitSimultaneous::execConcrete() {
  const auto outputBaseName = getOutputBaseName();

  if (getPropertyValue("OutputParameterWorkspace").empty())
    setProperty("OutputParameterWorkspace", outputBaseName + "_Parameters");

  if (getPropertyValue("OutputWorkspaceGroup").empty())
    setProperty("OutputWorkspaceGroup", outputBaseName + "_Workspaces");

  auto inputWorkspaces = getWorkspaces();
  auto workspaces = convertInputToElasticQ(inputWorkspaces);
  auto workspaceIndices = getWorkspaceIndices(workspaces.size());

  auto parameterWs = performFit(inputWorkspaces, outputBaseName);
  auto resultWs = processIndirectFitParameters(parameterWs);
  auto groupWs = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      outputBaseName + "_Workspaces");
  AnalysisDataService::Instance().addOrReplace(
      getPropertyValue("OutputWorkspace"), resultWs);

  if (containsMultipleData(workspaces))
    renameWorkspaces(groupWs, parameterWs, workspaceIndices, inputWorkspaces);
  else
    renameWorkspaces(groupWs, parameterWs, workspaceIndices);
  copyLogs(resultWs, workspaces);

  const bool doExtractMembers = getProperty("ExtractMembers");
  if (doExtractMembers)
    extractMembers(groupWs, workspaces, outputBaseName + "_Members");

  deleteTemporaryWorkspaces(outputBaseName);

  addAdditionalLogs(resultWs);
  copyLogs(resultWs, groupWs);

  setProperty("OutputWorkspace", resultWs);
  setProperty("OutputParameterWorkspace", parameterWs);
  setProperty("OutputWorkspaceGroup", groupWs);
}

ITableWorkspace_sptr QENSFitSimultaneous::performFit(
    const std::vector<MatrixWorkspace_sptr> &workspaces,
    const std::string &output) {
  IFunction_sptr function = getProperty("Function");
  bool extractMembers = getProperty("ExtractMembers");
  bool convolveMembers = getProperty("ConvolveMembers");
  bool passWsIndex = getProperty("PassWSIndexToFunction");
  bool ignoreInvalidData = getProperty("IgnoreInvalidData");
  bool calcErrors = getProperty("CalcErrors");
  bool outputParametersOnly = getProperty("OutputParametersOnly");

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
  fit->setPropertyValue("CalcErrors", getPropertyValue("CalcErrors"));
  fit->setProperty("OutputCompositeMembers", true);
  fit->setProperty("ConvolveMembers", convolveMembers);
  fit->setProperty("CreateOutput", true);
  fit->setProperty("Output", output);
  fit->executeAsChildAlg();
  return AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
      output + "_Parameters");
}

MatrixWorkspace_sptr QENSFitSimultaneous::processIndirectFitParameters(
    ITableWorkspace_sptr parameterWorkspace) {
  auto pifp =
      createChildAlgorithm("ProcessIndirectFitParameters", 0.91, 0.95, true);
  pifp->setProperty("InputWorkspace", parameterWorkspace);
  pifp->setProperty("ColumnX", "axis-1");
  pifp->setProperty("XAxisUnit", "MomentumTransfer");
  pifp->setProperty("ParameterNames", getFitParameterNames());
  pifp->setProperty("OutputWorkspace", "__Result");
  pifp->executeAsChildAlg();
  return pifp->getProperty("OutputWorkspace");
}

void QENSFitSimultaneous::renameWorkspaces(
    API::WorkspaceGroup_sptr outputGroup,
    API::ITableWorkspace_sptr parameterWorkspace,
    const std::vector<std::string> &indices) {
  auto rename = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  auto getNameSuffix = [&](std::size_t i) { return indices[i] + "_Workspace"; };
  return renameWorkspacesInQENSFit(this, rename, outputGroup,
                                   parameterWorkspace, getNameSuffix);
}

void QENSFitSimultaneous::renameWorkspaces(
    API::WorkspaceGroup_sptr outputGroup,
    API::ITableWorkspace_sptr parameterWorkspace,
    const std::vector<std::string> &indices,
    const std::vector<API::MatrixWorkspace_sptr> &workspaces) {
  auto rename = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  auto getNameSuffix = [&](std::size_t i) {
    return workspaces[i]->getName() + "_" + indices[i] + "_Workspace";
  };
  return renameWorkspacesInQENSFit(this, rename, outputGroup,
                                   parameterWorkspace, getNameSuffix);
}

void QENSFitSimultaneous::copyLogs(
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<MatrixWorkspace_sptr> &workspaces) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("OutputWorkspace", resultWorkspace->getName());

  for (const auto &workspace : workspaces) {
    logCopier->setProperty("InputWorkspace", workspace);
    logCopier->executeAsChildAlg();
  }
}

void QENSFitSimultaneous::copyLogs(MatrixWorkspace_sptr resultWorkspace,
                                   WorkspaceGroup_sptr resultGroup) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("InputWorkspace", resultWorkspace);
  logCopier->setProperty("OutputWorkspace", resultGroup->getName());
  logCopier->executeAsChildAlg();
}

void QENSFitSimultaneous::extractMembers(
    WorkspaceGroup_sptr resultGroupWs,
    const std::vector<MatrixWorkspace_sptr> &workspaces,
    const std::string &outputWsName) {
  std::vector<std::string> workspaceNames;
  std::transform(
      workspaces.begin(), workspaces.end(), std::back_inserter(workspaceNames),
      [](API::MatrixWorkspace_sptr workspace) { return workspace->getName(); });

  auto extractAlgorithm = extractMembersAlgorithm(resultGroupWs, outputWsName);
  extractAlgorithm->setProperty("InputWorkspaces", workspaceNames);
  extractAlgorithm->execute();
}

void QENSFitSimultaneous::deleteTemporaryWorkspaces(
    const std::string &outputBaseName) {
  auto deleter = createChildAlgorithm("DeleteWorkspace", -1.0, -1.0, false);
  deleter->setProperty("Workspace",
                       outputBaseName + "_NormalisedCovarianceMatrices");
  deleter->executeAsChildAlg();

  deleter->setProperty("Workspace", outputBaseName + "_Parameters");
  deleter->executeAsChildAlg();

  deleteTemporaries(deleter, getTemporaryName());
}

void QENSFitSimultaneous::addAdditionalLogs(
    MatrixWorkspace_sptr resultWorkspace) {
  auto logAdder = createChildAlgorithm("AddSampleLog", -1.0, -1.0, false);
  logAdder->setProperty("Workspace", resultWorkspace);

  Progress logAdderProg(this, 0.99, 1.00, 6);
  logAdder->setProperty("LogType", "String");
  for (const auto log : getAdditionalLogStrings()) {
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add text logs");
  }

  logAdder->setProperty("LogType", "Number");
  for (const auto log : getAdditionalLogNumbers()) {
    logAdder->setProperty("LogName", log.first);
    logAdder->setProperty("LogText", log.second);
    logAdder->executeAsChildAlg();
    logAdderProg.report("Add number logs");
  }
}

IAlgorithm_sptr QENSFitSimultaneous::extractMembersAlgorithm(
    WorkspaceGroup_sptr resultGroupWs, const std::string &outputWsName) const {
  bool convolved = getProperty("ConvolveMembers");
  std::vector<std::string> convolvedMembers;
  IFunction_sptr function = getProperty("Function");

  if (convolved)
    extractConvolvedNames(function, convolvedMembers);

  auto extractMembersAlg =
      AlgorithmManager::Instance().create("ExtractQENSMembers");
  extractMembersAlg->setProperty("ResultWorkspace", resultGroupWs);
  extractMembersAlg->setProperty("OutputWorkspace", outputWsName);
  extractMembersAlg->setProperty("RenameConvolvedMembers", convolved);
  extractMembersAlg->setProperty("ConvolvedMembers", convolvedMembers);
  return extractMembersAlg;
}

std::vector<MatrixWorkspace_sptr> QENSFitSimultaneous::getWorkspaces() const {
  std::vector<MatrixWorkspace_sptr> workspaces;
  workspaces.reserve(m_workspacePropertyNames.size());
  for (const auto &propertyName : m_workspacePropertyNames)
    workspaces.push_back(getProperty(propertyName));
  return workspaces;
}

std::vector<MatrixWorkspace_sptr> QENSFitSimultaneous::convertInputToElasticQ(
    const std::vector<MatrixWorkspace_sptr> &workspaces) const {
  return convertToElasticQ(workspaces, getTemporaryName(),
                           throwIfElasticQConversionFails());
}

std::string QENSFitSimultaneous::getOutputBaseName() const {
  const auto base = getPropertyValue("OutputWorkspace");
  auto position = base.rfind("_Result");
  if (position != std::string::npos)
    return base.substr(0, position);
  return base;
}

bool QENSFitSimultaneous::throwIfElasticQConversionFails() const {
  return false;
}

bool QENSFitSimultaneous::isFitParameter(const std::string &) const {
  return true;
}

std::vector<std::string> QENSFitSimultaneous::getFitParameterNames() const {
  IFunction_sptr function = getProperty("Function");

  std::unordered_set<std::string> nameSet;
  std::vector<std::string> names;
  names.reserve(function->nParams());

  for (auto i = 0u; i < function->nParams(); ++i) {
    auto name = shortParameterName(function->parameterName(i));

    if (isFitParameter(name) && nameSet.find(name) == nameSet.end()) {
      names.emplace_back(name);
      nameSet.insert(name);
    }
  }
  return names;
}

std::map<std::string, std::string>
QENSFitSimultaneous::getAdditionalLogStrings() const {
  bool convolve = getProperty("ConvolveMembers");
  auto fitProgram = name();
  fitProgram = fitProgram.substr(0, fitProgram.rfind("Simultaneous"));

  auto logs = std::map<std::string, std::string>();
  logs["convolve_members"] = convolve ? "true" : "false";
  logs["fit_program"] = fitProgram;
  logs["fit_mode"] = "Simultaneous";
  return logs;
}

std::map<std::string, std::string>
QENSFitSimultaneous::getAdditionalLogNumbers() const {
  return std::map<std::string, std::string>();
}

ITableWorkspace_sptr QENSFitSimultaneous::processParameterTable(
    ITableWorkspace_sptr parameterTable) const {
  return parameterTable;
}

std::vector<std::string>
QENSFitSimultaneous::getWorkspaceIndices(std::size_t numberOfIndices) const {
  std::vector<std::string> indices;
  indices.reserve(numberOfIndices);
  indices.push_back(getPropertyValue("WorkspaceIndex"));
  for (auto i = 1u; i < numberOfIndices; ++i)
    indices.push_back(getPropertyValue("WorkspaceIndex_" + std::to_string(i)));
  return indices;
}

std::string QENSFitSimultaneous::getTemporaryName() const {
  return "__" + name() + "_ws";
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
