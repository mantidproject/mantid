// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ReplaceIndirectFitResultBin.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"

#include <algorithm>
#include <boost/cast.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {

std::vector<double> getValuesInEBin(MatrixWorkspace_const_sptr workspace,
                                    std::size_t const &binIndex) {
  auto const numberOfHistograms(workspace->getNumberHistograms());

  std::vector<double> binValues;
  binValues.reserve(numberOfHistograms);
  for (auto i = 0u; i < numberOfHistograms; ++i)
    binValues.emplace_back(workspace->e(i)[binIndex]);
  return binValues;
}

std::vector<double> getValuesInYBin(MatrixWorkspace_const_sptr workspace,
                                    std::size_t const &binIndex) {
  auto const numberOfHistograms(workspace->getNumberHistograms());

  std::vector<double> binValues;
  binValues.reserve(numberOfHistograms);
  for (auto i = 0u; i < numberOfHistograms; ++i)
    binValues.emplace_back(workspace->y(i)[binIndex]);
  return binValues;
}

std::size_t getBinIndexOfValue(NumericAxis const *axis, double value) {
  auto const axisValues = axis->getValues();
  auto const iter = std::find(axisValues.begin(), axisValues.end(), value);
  if (iter != axisValues.end())
    return axisValues.end() - iter - 1;
  else
    throw std::runtime_error(
        "The corresponding bin in the input workspace could not be found.");
}

std::size_t getBinIndexOfValue(MatrixWorkspace_const_sptr workspace,
                               double value) {
  auto const axis = workspace->getAxis(0);
  if (axis->isNumeric()) {
    auto const *numericAxis = dynamic_cast<NumericAxis *>(axis);
    return getBinIndexOfValue(numericAxis, value);
  } else
    throw std::runtime_error(
        "The input workspace does not have a numeric x axis.");
}

std::size_t getBinIndex(MatrixWorkspace_const_sptr singleBinWorkspace,
                        MatrixWorkspace_sptr outputWorkspace) {
  auto const axis = singleBinWorkspace->getAxis(0);
  if (axis->isNumeric()) {
    auto const *numericAxis = dynamic_cast<NumericAxis *>(axis);
    auto const binValue = numericAxis->getValue(0);
    return getBinIndexOfValue(outputWorkspace, binValue);
  } else
    throw std::runtime_error(
        "The single bin workspace does not have a numeric x axis.");
}

template <typename Histo>
auto replaceBinValue(Histo histogram, std::size_t const &binIndex,
                     double newValue) {
  histogram[binIndex] = newValue;
  return histogram;
}

HistogramE replaceEBinValue(MatrixWorkspace_const_sptr workspace,
                            std::size_t const &spectrumIndex,
                            std::size_t const &binIndex, double newValue) {
  auto histogram = workspace->e(spectrumIndex);
  return replaceBinValue(histogram, binIndex, newValue);
}

HistogramY replaceYBinValue(MatrixWorkspace_const_sptr workspace,
                            std::size_t const &spectrumIndex,
                            std::size_t const &binIndex, double newValue) {
  auto histogram = workspace->y(spectrumIndex);
  return replaceBinValue(histogram, binIndex, newValue);
}

void replaceBinValues(MatrixWorkspace_sptr outputWorkspace,
                      std::size_t const &binIndex, std::vector<double> yValues,
                      std::vector<double> eValues) {
  for (auto i = 0u; i < outputWorkspace->getNumberHistograms(); ++i) {
    outputWorkspace->mutableY(i) =
        replaceYBinValue(outputWorkspace, i, binIndex, yValues[i]);
    outputWorkspace->mutableE(i) =
        replaceEBinValue(outputWorkspace, i, binIndex, eValues[i]);
  }
}

void processBinReplacement(MatrixWorkspace_const_sptr singleBinWorkspace,
                           MatrixWorkspace_sptr outputWorkspace) {
  auto const yValues = getValuesInYBin(singleBinWorkspace, 0);
  auto const eValues = getValuesInEBin(singleBinWorkspace, 0);
  auto const insertionIndex = getBinIndex(singleBinWorkspace, outputWorkspace);
  replaceBinValues(outputWorkspace, insertionIndex, yValues, eValues);
}

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

Workspace_sptr getADSWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<Workspace>(workspaceName);
}

template <typename Predicate>
void removeVectorElements(std::vector<std::string> &strings,
                          Predicate const &filter) {
  strings.erase(std::remove_if(strings.begin(), strings.end(), filter),
                strings.end());
}

bool doesStringEndWith(std::string const &str, std::string const &delimiter) {
  if (str.size() > delimiter.size())
    return str.substr(str.size() - delimiter.size(), str.size()) == delimiter;
  return false;
}

std::vector<std::string> filterByEndSuffix(std::vector<std::string> &strings,
                                           std::string const &delimiter) {
  removeVectorElements(strings, [&delimiter](std::string const &str) {
    return !doesStringEndWith(str, delimiter);
  });
  return strings;
}

bool doesGroupContain(std::string const &groupName,
                      MatrixWorkspace_sptr workspace) {
  auto const adsWorkspace = getADSWorkspace(groupName);
  if (adsWorkspace->isGroup()) {
    auto const group =
        boost::dynamic_pointer_cast<WorkspaceGroup>(adsWorkspace);
    return group->contains(workspace);
  }
  return false;
}

std::string filterByContents(std::vector<std::string> &strings,
                             MatrixWorkspace_sptr workspace) {
  removeVectorElements(strings, [&workspace](std::string const &str) {
    return !doesGroupContain(str, workspace);
  });
  return !strings.empty() ? strings[0] : "";
}

std::string findResultGroupContaining(MatrixWorkspace_sptr workspace) {
  auto resultGroups = filterByEndSuffix(
      AnalysisDataService::Instance().getObjectNames(), "_Results");
  return !resultGroups.empty() ? filterByContents(resultGroups, workspace) : "";
}

WorkspaceGroup_sptr getADSGroupWorkspace(std::string const &workspaceName) {
  return boost::dynamic_pointer_cast<WorkspaceGroup>(
      getADSWorkspace(workspaceName));
}

void addOutputToResultsGroup(WorkspaceGroup_sptr resultGroup,
                             std::string const &inputName,
                             std::string const &outputName,
                             MatrixWorkspace_sptr output) {
  if (inputName == outputName)
    resultGroup->remove(inputName);
  resultGroup->addWorkspace(output);
}

void addOutputToResultsGroup(std::string const &resultGroupName,
                             std::string const &inputName,
                             std::string const &outputName,
                             MatrixWorkspace_sptr output) {
  if (!resultGroupName.empty())
    addOutputToResultsGroup(getADSGroupWorkspace(resultGroupName), inputName,
                            outputName, output);
  else
    throw std::runtime_error(
        "The input workspaces corresponding result group could not be found.");
}

} // namespace

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(ReplaceIndirectFitResultBin)

/// Algorithms name for identification. @see Algorithm::name
const std::string ReplaceIndirectFitResultBin::name() const {
  return "ReplaceIndirectFitResultBin";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReplaceIndirectFitResultBin::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReplaceIndirectFitResultBin::category() const {
  return "Inelastic\\Indirect";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReplaceIndirectFitResultBin::summary() const {
  return "During a sequential fit in Indirect Data Analysis, the parameters "
         "fitted for a spectrum become the start parameters for the next "
         "spectrum. This can be a problem if the next spectrum is not similar "
         "to the previous spectrum and will lead to a poor fit for that "
         "spectrum.\n"
         "This algorithm takes a result workspace of a sequential fit for "
         "multiple spectra (1), and a result workspace for a singly fit "
         "spectrum(2) and it will replace the corresponding bad bin value in "
         "workspace (1) with the bin found in workspace (2).\n"
         "Note that workspaces (1) and (2) should be result workspaces "
         "generated by a fit with the same fit functions and minimizers. Also "
         "note that the output workspace is inserted back into the result "
         "workspace group in which the Input workspace is found.\n";
}

void ReplaceIndirectFitResultBin::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The result workspace containing the poor fit value which "
                  "needs replacing. It's name must end with _Result.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "SingleBinWorkspace", "", Direction::Input),
                  "The result workspace containing the replacement bin. It's "
                  "name must end with _Result.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to give the output workspace.");
}

std::map<std::string, std::string>
ReplaceIndirectFitResultBin::validateInputs() {
  std::map<std::string, std::string> errors;

  auto const inputWorkspaceName = getPropertyValue("InputWorkspace");
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  auto const singleBinWorkspaceName = getPropertyValue("SingleBinWorkspace");
  MatrixWorkspace_sptr singleBinWorkspace = getProperty("SingleBinWorkspace");
  auto const outputWorkspaceName = getPropertyValue("OutputWorkspace");

  if (inputWorkspaceName.empty())
    errors["InputWorkspace"] = "No input workspace was provided.";
  else if (!doesStringEndWith(inputWorkspaceName, "_Result"))
    errors["InputWorkspace"] =
        "The input workspace must be a result workspace ending in _Result.";
  else if (inputWorkspace->y(0).size() < 2)
    errors["InputWorkspace"] =
        "The input workspace must contain 2 or more bins.";

  if (singleBinWorkspaceName.empty())
    errors["SingleBinWorkspace"] =
        "No single fit result workspace was provided.";
  else if (!doesStringEndWith(singleBinWorkspaceName, "_Result"))
    errors["SingleBinWorkspace"] = "The single bin workspace must be a result "
                                   "workspace ending in _Result.";
  else if (singleBinWorkspace->y(0).size() > 1)
    errors["SingleBinWorkspace"] =
        "The single bin workspace must contain only 1 bin.";

  if (singleBinWorkspace->getNumberHistograms() !=
      inputWorkspace->getNumberHistograms())
    errors["InputWorkspace"] =
        "The input workspace and single bin workspace must "
        "have the same number of histograms.";

  if (outputWorkspaceName.empty())
    errors["OutputWorkspace"] = "No OutputWorkspace name was provided.";

  try {
    (void)getBinIndex(singleBinWorkspace, inputWorkspace);
  } catch (std::exception const &ex) {
    errors["InputWorkspace"] = ex.what();
  }

  return errors;
}

void ReplaceIndirectFitResultBin::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr singleBinWorkspace =
      getProperty("SingleBinWorkspace");
  auto const outputName = getPropertyValue("OutputWorkspace");

  auto const inputName = inputWorkspace->getName();

  auto outputWorkspace = cloneWorkspace(inputName);
  processBinReplacement(singleBinWorkspace, outputWorkspace);

  setProperty("OutputWorkspace", outputWorkspace);
  addOutputToResultsGroup(findResultGroupContaining(inputWorkspace), inputName,
                          outputName, outputWorkspace);
}

MatrixWorkspace_sptr
ReplaceIndirectFitResultBin::cloneWorkspace(std::string const &inputName) {
  auto cloneAlg = createChildAlgorithm("CloneWorkspace", -1.0, -1.0, false);
  cloneAlg->initialize();
  cloneAlg->setProperty("InputWorkspace", inputName);
  cloneAlg->setProperty("OutputWorkspace", "__cloned");
  cloneAlg->executeAsChildAlg();
  Workspace_sptr outputWorkspace = cloneAlg->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<MatrixWorkspace>(outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
