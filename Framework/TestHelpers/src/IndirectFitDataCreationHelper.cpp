// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

namespace Mantid {
namespace IndirectFitDataCreationHelper {
using namespace Mantid::API;

MatrixWorkspace_sptr createWorkspace(int const &numberOfSpectra,
                                     int const &numberOfBins) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfSpectra,
                                                    numberOfBins);
}

MatrixWorkspace_sptr createInstrumentWorkspace(int const &xLength,
                                               int const &yLength) {
  return WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
      xLength, yLength - 1, false, false, true, "testInst");
}

MatrixWorkspace_sptr
createWorkspaceWithTextAxis(int const &numberOfSpectra,
                            std::vector<std::string> const &labels,
                            int const &numberOfBins) {
  if (static_cast<std::size_t>(numberOfSpectra) == labels.size()) {
    auto workspace = createWorkspace(numberOfSpectra, numberOfBins);
    workspace->replaceAxis(1, getTextAxis(numberOfSpectra, labels));
    return workspace;
  } else
    throw std::runtime_error(
        "The number of spectra is not equal to the number of labels");
}

MatrixWorkspace_sptr
createWorkspaceWithBinValues(int const &numberOfSpectra,
                             std::vector<double> const &values,
                             int const &numberOfBins) {
  if (static_cast<std::size_t>(numberOfBins) == values.size()) {
    auto workspace = createWorkspace(numberOfSpectra, numberOfBins);
    workspace->replaceAxis(0, getNumericAxis(numberOfBins, values));
    return workspace;
  } else
    throw std::runtime_error(
        "The number of bins is not equal to the number of labels");
}

WorkspaceGroup_sptr createGroupWorkspace(std::size_t const &numberOfWorkspaces,
                                         int const &numberOfSpectra,
                                         int const &numberOfBins) {
  auto groupWorkspace = boost::make_shared<WorkspaceGroup>();
  for (auto i = 0u; i < numberOfWorkspaces; ++i)
    groupWorkspace->addWorkspace(
        createWorkspace(numberOfSpectra, numberOfBins));
  return groupWorkspace;
}

WorkspaceGroup_sptr
createGroupWorkspaceWithTextAxes(std::size_t const &numberOfWorkspaces,
                                 std::vector<std::string> const &labels,
                                 int const &numberOfSpectra,
                                 int const &numberOfBins) {
  auto groupWorkspace = boost::make_shared<WorkspaceGroup>();
  for (auto i = 0u; i < numberOfWorkspaces; ++i)
    groupWorkspace->addWorkspace(
        createWorkspaceWithTextAxis(numberOfSpectra, labels, numberOfBins));
  return groupWorkspace;
}

TextAxis *getTextAxis(int const &numberOfSpectra,
                      std::vector<std::string> const &labels) {
  auto axis = new TextAxis(numberOfSpectra);
  for (auto index = 0; index < numberOfSpectra; ++index)
    axis->setLabel(index, labels[index]);
  return axis;
}

NumericAxis *getNumericAxis(int const &numberOfLabels,
                            std::vector<double> const &values) {
  auto axis = new NumericAxis(numberOfLabels);
  for (auto index = 0; index < numberOfLabels; ++index)
    axis->setValue(index, values[index]);
  return axis;
}

MatrixWorkspace_sptr setWorkspaceEFixed(MatrixWorkspace_sptr workspace,
                                        int const &xLength) {
  for (int i = 0; i < xLength; ++i)
    workspace->setEFixed((i + 1), 0.50);
  return workspace;
}

MatrixWorkspace_sptr
setWorkspaceBinEdges(MatrixWorkspace_sptr workspace, int const &yLength,
                     Mantid::HistogramData::BinEdges const &binEdges) {
  for (int i = 0; i < yLength; ++i)
    workspace->setBinEdges(i, binEdges);
  return workspace;
}

MatrixWorkspace_sptr setWorkspaceBinEdges(MatrixWorkspace_sptr workspace,
                                          int const &xLength,
                                          int const &yLength) {
  Mantid::HistogramData::BinEdges binEdges(xLength - 1, 0.0);
  int j = 0;
  std::generate(begin(binEdges), end(binEdges),
                [&j] { return 0.5 + 0.75 * ++j; });
  setWorkspaceBinEdges(workspace, yLength, binEdges);
  return workspace;
}

MatrixWorkspace_sptr setWorkspaceProperties(MatrixWorkspace_sptr workspace,
                                            int const &xLength,
                                            int const &yLength) {
  setWorkspaceBinEdges(workspace, xLength, yLength);
  setWorkspaceEFixed(workspace, xLength);
  return workspace;
}

MatrixWorkspace_sptr createWorkspaceWithInstrument(int const &xLength,
                                                   int const &yLength) {
  auto workspace = createInstrumentWorkspace(xLength, yLength);
  workspace->initialize(yLength, xLength, xLength - 1);
  return setWorkspaceProperties(workspace, xLength, yLength);
}

} // namespace IndirectFitDataCreationHelper
} // namespace Mantid
