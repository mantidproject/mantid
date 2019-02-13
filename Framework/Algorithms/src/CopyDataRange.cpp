// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CopyDataRange.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/BoundedValidator.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {

void copyDataRange(MatrixWorkspace_const_sptr inputWorkspace,
                   MatrixWorkspace_sptr destWorkspace, int const &specMin,
                   int const &specMax, int const &xMin, int const &xMax,
                   int yInsertionIndex, int const &xInsertionIndex) {
  for (auto specIndex = specMin; specIndex <= specMax; ++specIndex) {
    std::copy(inputWorkspace->y(specIndex).begin() + xMin,
              inputWorkspace->y(specIndex).begin() + xMax,
              destWorkspace->mutableY(yInsertionIndex).begin() +
                  xInsertionIndex);
    std::copy(inputWorkspace->e(specIndex).begin() + xMin,
              inputWorkspace->e(specIndex).begin() + xMax,
              destWorkspace->mutableE(yInsertionIndex).begin() +
                  xInsertionIndex);
    ++yInsertionIndex;
  }
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
  declareProperty("StartWorkspaceIndex", 0, positiveInt,
                  "The index denoting the start of the spectra range.");

  declareProperty("EndWorkspaceIndex", EMPTY_INT(), positiveInt,
                  "The index denoting the end of the spectra range.");

  declareProperty("XMinIndex", 0, positiveInt,
                  "The index denoting the start of the x range.");

  declareProperty("XMaxIndex", EMPTY_INT(), positiveInt,
                  "The index denoting the end of the x range.");

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

  auto const inputWorkspaceName = getPropertyValue("InputWorkspace");
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  auto const destWorkspaceName = getPropertyValue("DestWorkspace");
  MatrixWorkspace_sptr destWorkspace = getProperty("DestWorkspace");
  auto const outputWorkspaceName = getPropertyValue("OutputWorkspace");

  int const specMinIndex = getProperty("StartWorkspaceIndex");
  int const specMaxIndex = getProperty("EndWorkspaceIndex");
  int const xMinIndex = getProperty("XMinIndex");
  int const xMaxIndex = getProperty("XMaxIndex");

  int const yInsertionIndex = getProperty("InsertionYIndex");
  int const xInsertionIndex = getProperty("InsertionXIndex");

  if (inputWorkspaceName.empty())
    errors["InputWorkspace"] = "No input workspace was provided.";

  if (destWorkspaceName.empty())
    errors["DestWorkspace"] = "No destination workspace was provided.";

  if (xMinIndex < 0)
    errors["XMinIndex"] = "The XMinIndex must be a number above zero.";
  if (xMaxIndex > inputWorkspace->y(0).size())
    errors["XMaxIndex"] = "The XMaxIndex is larger than the maximum range in "
                          "the input workspace.";
  if (xMinIndex > xMaxIndex)
    errors["XMinIndex"] = "The XMinIndex must be smaller than XMaxIndex.";

  if (specMinIndex < 0)
    errors["StartWorkspaceIndex"] =
        "The StartWorkspaceIndex must be a number above zero.";
  if (specMaxIndex >= inputWorkspace->getNumberHistograms())
    errors["EndWorkspaceIndex"] =
        "The EndWorkspaceIndex is larger than the number of histograms in "
        "the input workspace.";
  if (specMinIndex > specMaxIndex)
    errors["StartWorkspaceIndex"] =
        "The StartWorkspaceIndex must be smaller than the EndWorkspaceIndex.";

  if (destWorkspace->getNumberHistograms() <
      yInsertionIndex + specMaxIndex - specMinIndex)
    errors["InsertionYIndex"] =
        "The y data range selected will not fit into the "
        "destination workspace.";
  if (destWorkspace->y(0).size() < xInsertionIndex + xMaxIndex - xMinIndex)
    errors["InsertionXIndex"] =
        "The x data range selected will not fit into the "
        "destination workspace.";

  if (outputWorkspaceName.empty())
    errors["OutputWorkspace"] = "No OutputWorkspace name was provided.";

  return errors;
}

void CopyDataRange::exec() {
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  MatrixWorkspace_const_sptr destWorkspace = getProperty("DestWorkspace");
  int const specMinIndex = getProperty("StartWorkspaceIndex");
  int const specMaxIndex = getProperty("EndWorkspaceIndex");
  int const xMinIndex = getProperty("XMinIndex");
  int const xMaxIndex = getProperty("XMaxIndex");
  int const yInsertionIndex = getProperty("InsertionYIndex");
  int const xInsertionIndex = getProperty("InsertionXIndex");

  auto outputWorkspace = cloneWorkspace(destWorkspace->getName());
  copyDataRange(inputWorkspace, outputWorkspace, specMinIndex, specMaxIndex,
                xMinIndex, xMaxIndex, yInsertionIndex, xInsertionIndex);

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
