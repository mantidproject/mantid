#include "MantidWorkflowAlgorithms/QENSFitSequential.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include <boost/cast.hpp>
#include <boost/regex.hpp>

#include <sstream>
#include <unordered_map>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("QENSFitSequential");

void extractConvolvedNames(CompositeFunction_sptr function,
                           std::vector<std::string> &names);
void extractConvolvedNames(IFunction_sptr function,
                           std::vector<std::string> &names);

void extractConvolvedNamesFromConvolution(CompositeFunction_sptr convolution,
                                          std::vector<std::string> &names) {
  for (auto i = 0u; i < convolution->nFunctions(); ++i) {
    auto composite = boost::dynamic_pointer_cast<CompositeFunction>(
        convolution->getFunction(i));
    if (composite)
      extractConvolvedNames(composite, names);
    else
      names.emplace_back(convolution->getFunction(i)->name());
  }
}

void extractConvolvedNames(CompositeFunction_sptr function,
                           std::vector<std::string> &names) {
  if (function->name() == "Convolution")
    extractConvolvedNamesFromConvolution(function, names);
  else {
    for (auto i = 0u; i < function->nFunctions(); ++i)
      extractConvolvedNames(function->getFunction(i), names);
  }
}

void extractConvolvedNames(IFunction_sptr function,
                           std::vector<std::string> &names) {
  auto composite = boost::dynamic_pointer_cast<CompositeFunction>(function);
  if (composite)
    extractConvolvedNames(composite, names);
}

std::string constructInputString(MatrixWorkspace_sptr workspace, int specMin,
                                 int specMax) {
  std::ostringstream input;
  for (auto i = specMin; i < specMax + 1; ++i)
    input << workspace->getName() << ",i" << std::to_string(specMin)
          << std::to_string(specMax) << ";";
  return input.str();
}

std::vector<MatrixWorkspace_sptr> extractWorkspaces(const std::string &input) {
  std::vector<MatrixWorkspace_sptr> workspaces;

  auto extractWorkspace = [&](const std::string &name) {
    workspaces.emplace_back(
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name));
  };

  boost::regex reg("([^,;]+),");
  std::for_each(
      boost::sregex_token_iterator(input.begin(), input.end(), reg, 1),
      boost::sregex_token_iterator(), extractWorkspace);
  return workspaces;
}

std::vector<std::string> getSpectra(const std::string &input) {
  std::vector<std::string> spectra;
  boost::regex reg(",[i|sp](0|[1-9][0-9]*);");
  std::copy(boost::sregex_token_iterator(input.begin(), input.end(), reg, 1),
            boost::sregex_token_iterator(), std::back_inserter(spectra));
  return spectra;
}

std::vector<std::string> getSuffices(const std::string &input) {
  std::vector<std::string> suffices;
  boost::regex reg(",[i|sp](0|[1-9][0-9]*);");
  std::copy(boost::sregex_token_iterator(input.begin(), input.end(), reg, 0),
            boost::sregex_token_iterator(), std::back_inserter(suffices));
  return suffices;
}

std::string
replaceWorkspaces(const std::string &input,
                  const std::vector<MatrixWorkspace_sptr> &workspaces) {
  const auto suffices = getSuffices(input);
  std::stringstream newInput;
  for (auto i = 0u; i < workspaces.size(); ++i)
    newInput << workspaces[i]->getName() << suffices[i];
  return newInput.str();
}

std::string replaceWorkspace(const std::string &input,
                             const std::string &workspace) {
  const auto suffices = getSuffices(input);
  std::stringstream newInput;
  for (auto i = 0u; i < suffices.size(); ++i)
    newInput << workspace << suffices[i];
  return newInput.str();
}
} // namespace

namespace Mantid {
namespace Algorithms {

using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(QENSFitSequential)

/// Algorithms name for identification. @see Algorithm::name
const std::string QENSFitSequential::name() const {
  return "QENSFitSequential";
}

/// Algorithm's version for identification. @see Algorithm::version
int QENSFitSequential::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string QENSFitSequential::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string QENSFitSequential::summary() const {
  return "Performs a sequential fit for QENS data";
}

void QENSFitSequential::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       PropertyMode::Optional),
      "The input workspace for the fit. This property will be ignored if "
      "'Input' is provided.");

  auto boundedV = boost::make_shared<BoundedValidator<int>>();
  boundedV->setLower(0);

  declareProperty(
      "SpecMin", 0, boundedV,
      "The first spectrum to be used in "
      "the fit. Spectra values can not be "
      "negative. This property will be ignored if 'Input' is provided.",
      Direction::Input);

  declareProperty(
      "SpecMax", 0, boundedV,
      "The final spectrum to be used in "
      "the fit. Spectra values can not be "
      "negative. This property will be ignored if 'Input' is provided.",
      Direction::Input);

  declareProperty(
      "Input", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "A list of sources of data to fit. \n"
      "Sources can be either workspace names or file names followed optionally "
      "by a list of spectra/workspace-indices \n"
      "or values using the notation described in the description section of "
      "the help page.");

  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputParameterWorkspace", "", Direction::Output),
                  "The output parameter workspace");
  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputGroupWorkspace", "", Direction::Output),
                  "The output group workspace");
  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The output result workspace");
  declareProperty(
      make_unique<FunctionProperty>("Function", "", Direction::Input,
                                    functionValidator()),
      "The fitting function, common for all workspaces in the input.");
  declareProperty("LogValue", "",
                  "Name of the log value to plot the "
                  "parameters against. Default: use spectra "
                  "numbers.");
  declareProperty("StartX", EMPTY_DBL(),
                  "A value of x in, or on the low x "
                  "boundary of, the first bin to "
                  "include in\n"
                  "the fit (default lowest value of x)");
  declareProperty("EndX", EMPTY_DBL(),
                  "A value in, or on the high x boundary "
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

  std::vector<std::string> costFuncOptions =
      CostFunctionFactory::Instance().getKeys();
  declareProperty("CostFunction", "Least squares",
                  boost::make_shared<StringListValidator>(costFuncOptions),
                  "Cost functions to use for fitting. Cost functions available "
                  "are 'Least squares' and 'Ignore positive peaks'",
                  Direction::InOut);

  declareProperty("MaxIterations", 500,
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

  declareProperty("CreateOutput", false,
                  "Set to true to create output "
                  "workspaces with the results of the "
                  "fit(default is false).");

  declareProperty(
      "ExtractMembers", false,
      "If true, then each member of the convolution fit will be extracted"
      ", into their own workspace. These workspaces will have a histogram"
      " for each spectrum (Q-value) and will be grouped.",
      Direction::Input);

  declareProperty(
      make_unique<Kernel::PropertyWithValue<bool>>("ConvolveMembers", false),
      "If true and ExtractMembers is true members of any "
      "Convolution are output convolved\n"
      "with corresponding resolution");

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty(
      "EvaluationType", "CentrePoint",
      Kernel::IValidator_sptr(
          new Kernel::ListValidator<std::string>(evaluationTypes)),
      "The way the function is evaluated: CentrePoint or Histogram.",
      Kernel::Direction::Input);

  initConcrete();
}

IValidator_sptr QENSFitSequential::functionValidator() const {
  return boost::make_shared<MandatoryValidator<std::string>>();
}

void QENSFitSequential::initConcrete() {}

void QENSFitSequential::setup() {}

void QENSFitSequential::exec() {
  setup();
  auto workspaces = getWorkspaces();
  auto inputString = getInputString(workspaces);
  auto spectra = getSpectra(inputString);

  if (workspaces.size() != spectra.size() || workspaces.empty() ||
      spectra.empty())
    throw std::runtime_error("A malformed input string was provided.");

  auto outputWs = performFit(inputString);
  const auto outputBaseName = getPropertyValue("OutputWorkspace");

  deleteTemporaryWorkspaces(outputBaseName);

  auto resultWs = processIndirectFitParameters(outputWs);
  auto groupWs = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      outputBaseName + "_Workspaces");

  renameWorkspaces(groupWs, spectra);
  resultWs = copyLogs(resultWs, workspaces);

  const bool doExtractMembers = getProperty("ExtractMembers");
  if (doExtractMembers)
    extractMembers(groupWs, workspaces, outputBaseName);

  setProperty("OutputParameterWorkspace", outputWs);
  setProperty("OutputGroupWorkspace", groupWs);
  setProperty("OutputWorkspace", resultWs);
  postExec(resultWs);
}

void QENSFitSequential::postExec(MatrixWorkspace_sptr) {}

std::vector<std::string> QENSFitSequential::getFitParameterNames() const {
  IFunction_sptr function = getProperty("Function");
  return function->getParameterNames();
}

void QENSFitSequential::deleteTemporaryWorkspaces(
    const std::string &outputBaseName) {
  auto deleter = createChildAlgorithm("DeleteWorkspace", -1.0, -1.0, false);
  deleter->setProperty("Workspace",
                       outputBaseName + "_NormalisedCovarianceMatrices");
  deleter->executeAsChildAlg();

  deleter->setProperty("Workspace", outputBaseName + "_Parameters");
  deleter->executeAsChildAlg();
}

MatrixWorkspace_sptr QENSFitSequential::processIndirectFitParameters(
    ITableWorkspace_sptr parameterWorkspace) {
  auto pifp =
      createChildAlgorithm("ProcessIndirectFitParameters", 0.94, 0.96, true);
  pifp->setProperty("InputWorkspace", parameterWorkspace);
  pifp->setProperty("ColumnX", "axis-1");
  pifp->setProperty("XAxisUnit", "MomentumTransfer");
  pifp->setProperty("ParameterNames", getFitParameterNames());
  pifp->setProperty("OutputWorkspace", "__Result");
  pifp->executeAsChildAlg();
  return pifp->getProperty("OutputWorkspace");
}

void QENSFitSequential::renameWorkspaces(
    WorkspaceGroup_sptr outputGroup, const std::vector<std::string> &spectra) {
  auto renamer = createChildAlgorithm("RenameWorkspace", -1.0, -1.0, false);
  const auto groupNames = outputGroup->getNames();
  const std::string outputBase = getPropertyValue("OutputWorkspace");
  std::unordered_map<std::string, std::size_t> spectrumCount;

  Progress renamerProg(this, 0.98, 1.0, spectra.size());
  renamerProg.report("Renaming group workspaces...");

  for (auto i = 0u; i < spectra.size(); ++i) {
    renamer->setProperty("InputWorkspace", groupNames[i]);
    std::ostringstream name;
    auto count = spectrumCount.find(spectra[i]);

    if (count == spectrumCount.end()) {
      name << outputBase << spectra[i] << "_Workspace";
      spectrumCount[spectra[i]] = 1;
    } else
      name << outputBase << spectra[i] << "(" << ++count->second << ")"
           << "_Workspace";

    renamer->setProperty("OutputWorkspace", name.str());
    renamer->executeAsChildAlg();
    renamerProg.report("Renamed workspace in group.");
  }
}

ITableWorkspace_sptr QENSFitSequential::performFit(const std::string &input) {
  bool extractMembers = getProperty("ExtractMembers");
  bool convolveMembers = getProperty("ConvolveMembers");

  // Run PlotPeaksByLogValue
  auto plotPeaks = createChildAlgorithm("PlotPeakByLogValue", 0.05, 0.90, true);
  plotPeaks->setProperty("Input", input);
  plotPeaks->setProperty("OutputWorkspace",
                         getPropertyValue("OutputWorkspace"));
  plotPeaks->setProperty("Function", getPropertyValue("Function"));
  plotPeaks->setProperty("StartX", getPropertyValue("StartX"));
  plotPeaks->setProperty("EndX", getPropertyValue("EndX"));
  plotPeaks->setProperty("FitType", "Sequential");
  plotPeaks->setProperty("CreateOutput", getPropertyValue("CreateOutput"));
  plotPeaks->setProperty("OutputCompositeMembers", extractMembers);
  plotPeaks->setProperty("ConvolveMembers", convolveMembers);
  plotPeaks->setProperty("MaxIterations", getPropertyValue("MaxIterations"));
  plotPeaks->setProperty("Minimizer", getPropertyValue("Minimizer"));
  plotPeaks->setProperty("PassWSIndexToFunction",
                         getPropertyValue("PassWSIndexToFunction"));
  plotPeaks->setProperty("PeakRadius", getPropertyValue("PeakRadius"));
  plotPeaks->setProperty("LogValue", getPropertyValue("LogValue"));
  plotPeaks->setProperty("EvaluationType", getPropertyValue("EvaluationType"));
  plotPeaks->setProperty("CostFunction", getPropertyValue("CostFunction"));
  plotPeaks->executeAsChildAlg();
  return plotPeaks->getProperty("OutputWorkspace");
}

std::string QENSFitSequential::getInputString(
    const std::vector<MatrixWorkspace_sptr> &workspaces) const {
  auto inputString = getPropertyValue("Input");
  if (!inputString.empty())
    return replaceWorkspaces(inputString, workspaces);
  return constructInputString(workspaces[0], getProperty("SpecMin"),
                              getProperty("SpecMax"));
}

std::vector<MatrixWorkspace_sptr> QENSFitSequential::getWorkspaces() const {
  const auto inputString = getPropertyValue("Input");
  if (!inputString.empty())
    return extractWorkspaces(inputString);
  return {getProperty("InputWorkspace")};
}

void QENSFitSequential::extractMembers(
    WorkspaceGroup_sptr resultGroupWs,
    const std::vector<API::MatrixWorkspace_sptr> &workspaces,
    const std::string &outputWsName) {
  std::vector<std::string> workspaceNames;
  std::transform(
      workspaces.begin(), workspaces.end(), std::back_inserter(workspaceNames),
      [](API::MatrixWorkspace_sptr workspace) { workspace->getName(); });

  auto extractAlgorithm = extractMembersAlgorithm(resultGroupWs, outputWsName);
  extractAlgorithm->setProperty("InputWorkspaces", workspaceNames);
  extractAlgorithm->execute();
}

MatrixWorkspace_sptr QENSFitSequential::copyLogs(
    MatrixWorkspace_sptr resultWorkspace,
    const std::vector<MatrixWorkspace_sptr> &workspaces) {
  auto logCopier = createChildAlgorithm("CopyLogs", -1.0, -1.0, false);
  logCopier->setProperty("OutputWorkspace", resultWorkspace);

  for (const auto &workspace : workspaces) {
    logCopier->setProperty("InputWorkspace", workspace);
    logCopier->executeAsChildAlg();
  }
  return resultWorkspace;
}

IAlgorithm_sptr QENSFitSequential::extractMembersAlgorithm(
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

} // namespace Algorithms
} // namespace Mantid
