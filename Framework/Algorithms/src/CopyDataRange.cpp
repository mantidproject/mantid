// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyDataRange.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {

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
  auto const xMinIndex =
      static_cast<int>(inputWorkspace->yIndexOfX(xMin, 0, 0.000001));
  auto const xMaxIndex =
      static_cast<int>(inputWorkspace->yIndexOfX(xMax, 0, 0.000001));

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

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The workspace containing a range of data to be used for the "
                  "replacement.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "DestWorkspace", "", Direction::Input),
                  "The workspace to have range of data replaced.");

  auto const positiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(0);
  auto const anyDouble = boost::make_shared<Kernel::BoundedValidator<double>>();

  declareProperty("StartWorkspaceIndex", 0, positiveInt,
                  "The index denoting the start of the spectra range.");

  declareProperty("EndWorkspaceIndex", EMPTY_INT(), positiveInt,
                  "The index denoting the end of the spectra range.");

  declareProperty("XMin", EMPTY_DBL(), anyDouble,
                  "An X value that is within the first (lowest X value) bin");

  declareProperty("XMax", EMPTY_DBL(), anyDouble,
                  "An X value that is in the highest X value bin");

  declareProperty("InsertionYIndex", 0, positiveInt,
                  "The index denoting the histogram position for the start of "
                  "the data replacement in the DestWorkspace.");

  declareProperty("InsertionXIndex", 0, positiveInt,
                  "The index denoting the x position for the start of the data "
                  "replacement in the DestWorkspace.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
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

  try {
    auto const xMinIndex = inputWorkspace->yIndexOfX(xMin, 0, 0.000001);
    auto const xMaxIndex = inputWorkspace->yIndexOfX(xMax, 0, 0.000001);

    if (xMinIndex > xMaxIndex)
      errors["XMin"] = "XMin must come after XMax.";

    if (destWorkspace->y(0).size() <=
        static_cast<std::size_t>(xInsertionIndex) + xMaxIndex - xMinIndex)
      errors["InsertionXIndex"] = "The x data range selected will not fit into "
                                  "the destination workspace.";

  } catch (std::exception const &ex) {
    errors["XMin"] = ex.what();
    errors["XMax"] = ex.what();
  }

  if (specMaxIndex >= static_cast<int>(inputWorkspace->getNumberHistograms()))
    errors["EndWorkspaceIndex"] =
        "The EndWorkspaceIndex is larger than the number of histograms in the "
        "input workspace.";
  if (specMinIndex > specMaxIndex)
    errors["StartWorkspaceIndex"] =
        "The StartWorkspaceIndex must be smaller than the EndWorkspaceIndex.";

  if (static_cast<int>(destWorkspace->getNumberHistograms()) <=
      yInsertionIndex + specMaxIndex - specMinIndex)
    errors["InsertionYIndex"] = "The y data range selected will not fit into "
                                "the destination workspace.";

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
