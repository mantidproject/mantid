// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyDataRange.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/BoundedValidator.h"

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
  for (auto index = 0u; index < axis->length(); ++index)
    if (axis->getValue(index) == value)
      return index;

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

} // namespace

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CopyDataRange)

/// Algorithms name for identification. @see Algorithm::name
const std::string CopyDataRange::name() const { return "CopyDataRange"; }

/// Algorithm's version for identification. @see Algorithm::version
int CopyDataRange::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CopyDataRange::category() const {
  return "Inelastic\\Indirect";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CopyDataRange::summary() const {
  return "Replaces a range of data in the input workspace with a range of data "
         "from the copy workspace";
}

void CopyDataRange::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The workspace to have range of data replaced.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "CopyWorkspace", "", Direction::Input),
                  "The workspace containing a range of data to be used for the "
                  "replacement.");

  auto positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(0);
  declareProperty("SpecMin", EMPTY_INT(), positiveInt,
                  "The index denoting the start of the y range.");

  declareProperty("SpecMax", EMPTY_INT(), positiveInt,
                  "The index denoting the end of the y range.");

  declareProperty("XMin", EMPTY_INT(), positiveInt,
                  "The index denoting the start of the x range.");

  declareProperty("XMax", EMPTY_INT(), positiveInt,
                  "The index denoting the end of the x range.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to give the output workspace.");
}

std::map<std::string, std::string> CopyDataRange::validateInputs() {
  std::map<std::string, std::string> errors;

  auto const inputWorkspaceName = getPropertyValue("InputWorkspace");
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  auto const copyWorkspaceName = getPropertyValue("CopyWorkspace");
  MatrixWorkspace_sptr copyWorkspace = getProperty("CopyWorkspace");
  auto const outputWorkspaceName = getPropertyValue("OutputWorkspace");

  int const specMin = getProperty("SpecMin");
  int const specMax = getProperty("SpecMax");
  int const xMin = getProperty("XMin");
  int const xMax = getProperty("XMax");

  if (inputWorkspaceName.empty())
    errors["InputWorkspace"] = "No input workspace was provided.";

  if (copyWorkspaceName.empty())
    errors["CopyWorkspace"] = "No copy workspace was provided.";

  if (xMin < 0)
    errors["XMin"] = "XMin must be a number above zero.";
  if (xMax > inputWorkspace->y(0).size() || xMax > copyWorkspace->y(0).size())
    errors["XMax"] =
        "XMax is larger than the maximum range in the input or copy workspace.";
  if (xMin > xMax)
    errors["XMin"] = "XMin must be smaller than XMax.";

  if (specMin < 0)
    errors["SpecMin"] = "SpecMin must be a number above zero.";
  if (specMax >= inputWorkspace->getNumberHistograms() ||
      specMax >= copyWorkspace->getNumberHistograms())
    errors["SpecMax"] = "SpecMax is larger than the number of histograms in "
                        "the input or copy workspace.";
  if (specMin > specMax)
    errors["SpecMin"] = "SpecMin must be smaller than SpecMax.";

  if (outputWorkspaceName.empty())
    errors["OutputWorkspace"] = "No OutputWorkspace name was provided.";

  return errors;
}

void CopyDataRange::exec() {
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr copyWorkspace = getProperty("CopyWorkspace");
  auto const outputName = getPropertyValue("OutputWorkspace");

  auto const inputName = inputWorkspace->getName();

  auto outputWorkspace = cloneWorkspace(inputName);
  processBinReplacement(copyWorkspace, outputWorkspace);

  setProperty("OutputWorkspace", outputWorkspace);
}

MatrixWorkspace_sptr
CopyDataRange::cloneWorkspace(std::string const &inputName) {
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
