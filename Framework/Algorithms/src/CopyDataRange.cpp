// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyDataRange.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {

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

void copyDataRange(MatrixWorkspace_const_sptr inputWorkspace,
                   MatrixWorkspace_sptr destWorkspace, int const &specMin,
                   int const &specMax, int const &xMinIndex,
                   int const &xMaxIndex, int yInsertionIndex,
                   int const &xInsertionIndex) {
  for (auto specIndex = specMin; specIndex <= specMax; ++specIndex) {
    std::copy(inputWorkspace->y(specIndex).begin() + xMinIndex,
              inputWorkspace->y(specIndex).begin() + xMaxIndex + 1,
              destWorkspace->mutableY(yInsertionIndex).begin() +
                  xInsertionIndex);
    std::copy(inputWorkspace->e(specIndex).begin() + xMinIndex,
              inputWorkspace->e(specIndex).begin() + xMaxIndex + 1,
              destWorkspace->mutableE(yInsertionIndex).begin() +
                  xInsertionIndex);
    ++yInsertionIndex;
  }
}

void copyDataRange(MatrixWorkspace_const_sptr inputWorkspace,
                   MatrixWorkspace_sptr destWorkspace, int const &specMin,
                   int const &specMax, double const &xMin, double const &xMax,
                   int yInsertionIndex, int const &xInsertionIndex) {
  int const xMinIndex =
      static_cast<int>(getBinIndexOfValue(inputWorkspace, xMin));
  int const xMaxIndex =
      static_cast<int>(getBinIndexOfValue(inputWorkspace, xMax));
  copyDataRange(inputWorkspace, destWorkspace, specMin, specMax, xMinIndex,
                xMaxIndex, yInsertionIndex, xInsertionIndex);
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
  return "Utility\\Workspaces";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CopyDataRange::summary() const {
  return "Replaces a range of data in the destination workspace with a "
         "specified continuous range of data from the input workspace";
}

void CopyDataRange::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The workspace containing a range of data to be used for the "
                  "replacement.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "DestWorkspace", "", Direction::Input),
                  "The workspace to have range of data replaced.");

  auto const positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(0);
  auto const positiveDbl =
      boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveDbl->setLower(0.0);

  declareProperty("StartWorkspaceIndex", 0, positiveInt,
                  "The index denoting the start of the spectra range.");

  declareProperty("EndWorkspaceIndex", EMPTY_INT(), positiveInt,
                  "The index denoting the end of the spectra range.");

  declareProperty("XMin", EMPTY_DBL(), positiveDbl,
                  "An X value that is within the first (lowest X value) bin");

  declareProperty("XMax", EMPTY_DBL(), positiveDbl,
                  "An X value that is in the highest X value bin");

  declareProperty("InsertionYIndex", 0, positiveInt,
                  "The index denoting the histogram position for the start of "
                  "the data replacement in the DestWorkspace.");

  declareProperty("InsertionXIndex", 0, positiveInt,
                  "The index denoting the x position for the start of the data "
                  "replacement in the DestWorkspace.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to give the output workspace.");
}

std::map<std::string, std::string> CopyDataRange::validateInputs() {
  std::map<std::string, std::string> errors;

  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_sptr destWorkspace = getProperty("DestWorkspace");

  int const specMinIndex = getProperty("StartWorkspaceIndex");
  int const specMaxIndex = getProperty("EndWorkspaceIndex");
  double const xMin = getProperty("XMin");
  double const xMax = getProperty("XMax");

  int const yInsertionIndex = getProperty("InsertionYIndex");
  int const xInsertionIndex = getProperty("InsertionXIndex");

  if (getBinIndexOfValue(inputWorkspace, xMax) >= inputWorkspace->y(0).size())
    errors["XMax"] =
        "XMax is larger than the maximum range in the input workspace.";
  if (getBinIndexOfValue(inputWorkspace, xMin) >
      getBinIndexOfValue(inputWorkspace, xMax))
    errors["XMin"] = "XMin must come after XMax.";

  if (specMaxIndex >= static_cast<int>(inputWorkspace->getNumberHistograms()))
    errors["EndWorkspaceIndex"] =
        "The EndWorkspaceIndex is larger than the number of histograms in "
        "the input workspace.";
  if (specMinIndex > specMaxIndex)
    errors["StartWorkspaceIndex"] =
        "The StartWorkspaceIndex must be smaller than the EndWorkspaceIndex.";

  if (static_cast<int>(destWorkspace->getNumberHistograms()) <
      yInsertionIndex + specMaxIndex - specMinIndex)
    errors["InsertionYIndex"] =
        "The y data range selected will not fit into the "
        "destination workspace.";
  if (destWorkspace->y(0).size() <
      static_cast<std::size_t>(xInsertionIndex) +
          getBinIndexOfValue(inputWorkspace, xMax) -
          getBinIndexOfValue(inputWorkspace, xMin))
    errors["InsertionXIndex"] =
        "The x data range selected will not fit into the "
        "destination workspace.";

  return errors;
}

void CopyDataRange::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr destWorkspace = getProperty("DestWorkspace");
  int const specMinIndex = getProperty("StartWorkspaceIndex");
  int const specMaxIndex = getProperty("EndWorkspaceIndex");
  double const xMin = getProperty("XMin");
  double const xMax = getProperty("XMax");
  int const yInsertionIndex = getProperty("InsertionYIndex");
  int const xInsertionIndex = getProperty("InsertionXIndex");
  MatrixWorkspace_sptr outputWorkspace = getProperty("OutputWorkspace");

  if (destWorkspace != outputWorkspace)
    outputWorkspace = destWorkspace->clone();

  copyDataRange(inputWorkspace, outputWorkspace, specMinIndex, specMaxIndex,
                xMin, xMax, yInsertionIndex, xInsertionIndex);

  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Algorithms
} // namespace Mantid
