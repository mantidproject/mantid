// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/JoinISISPolarizationEfficiencies.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidHistogramData/LinearGenerator.h"

#include <limits>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(JoinISISPolarizationEfficiencies)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string JoinISISPolarizationEfficiencies::name() const {
  return "JoinISISPolarizationEfficiencies";
}

/// Algorithm's version for identification. @see Algorithm::version
int JoinISISPolarizationEfficiencies::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string JoinISISPolarizationEfficiencies::summary() const {
  return "Joins workspaces containing ISIS reflectometry polarization "
         "efficiency factors into a single workspace ready to be used with "
         "PolarizationEfficiencyCor.";
}

const std::vector<std::string>
JoinISISPolarizationEfficiencies::seeAlso() const {
  return {"CreatePolarizationEfficiencies", "LoadISISPolarizationEfficiencies",
          "PolarizationEfficiencyCor"};
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void JoinISISPolarizationEfficiencies::init() {

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Pp, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the Pp polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Ap, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the Ap polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Rho, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the Rho polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Alpha, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the Alpha polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          P1, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the P1 polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          P2, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the P2 polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          F1, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the F1 polarization efficiency.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          F2, "", Kernel::Direction::Input, PropertyMode::Optional),
      "A matrix workspaces containing the F2 polarization efficiency.");

  initOutputWorkspace();
}

/// Load efficientcies from files and put them into a single workspace.
/// @param props :: Names of properties containg names of files to load.
MatrixWorkspace_sptr JoinISISPolarizationEfficiencies::createEfficiencies(
    std::vector<std::string> const &props) {
  std::vector<MatrixWorkspace_sptr> workspaces;
  for (auto const &propName : props) {
    MatrixWorkspace_sptr ws = getProperty(propName);
    if (ws->getNumberHistograms() != 1) {
      throw std::runtime_error(
          "Loaded workspace must contain a single histogram. Found " +
          std::to_string(ws->getNumberHistograms()));
    }
    workspaces.push_back(ws);
  }

  return createEfficiencies(props, workspaces);
}

/// Create the efficiency workspace by combining single spectra workspaces into
/// one.
/// @param labels :: Axis labels for each workspace.
/// @param workspaces :: Workspaces to put together.
MatrixWorkspace_sptr JoinISISPolarizationEfficiencies::createEfficiencies(
    std::vector<std::string> const &labels,
    std::vector<MatrixWorkspace_sptr> const &workspaces) {
  auto interpolatedWorkspaces = interpolateWorkspaces(workspaces);

  auto const &inWS = interpolatedWorkspaces.front();
  MatrixWorkspace_sptr outWS = DataObjects::create<Workspace2D>(
      *inWS, labels.size(), inWS->histogram(0));
  auto axis1 = std::make_unique<TextAxis>(labels.size());
  outWS->replaceAxis(1, std::move(axis1));
  outWS->getAxis(0)->setUnit("Wavelength");

  for (size_t i = 0; i < interpolatedWorkspaces.size(); ++i) {
    auto &ws = interpolatedWorkspaces[i];
    outWS->setHistogram(i, ws->histogram(0));
    axis1->setLabel(i, labels[i]);
  }

  return outWS;
}

/// Interpolate the workspaces so that all have the same blocksize.
/// @param workspaces :: The workspaces to interpolate.
/// @return A list of interpolated workspaces.
std::vector<MatrixWorkspace_sptr>
JoinISISPolarizationEfficiencies::interpolateWorkspaces(
    std::vector<MatrixWorkspace_sptr> const &workspaces) {
  size_t minSize(std::numeric_limits<size_t>::max());
  size_t maxSize(0);
  bool thereAreHistograms = false;
  bool allAreHistograms = true;

  // Find out if the workspaces need to be interpolated.
  for (auto const &ws : workspaces) {
    auto size = ws->blocksize();
    if (size < minSize) {
      minSize = size;
    }
    if (size > maxSize) {
      maxSize = size;
    }
    thereAreHistograms = thereAreHistograms || ws->isHistogramData();
    allAreHistograms = allAreHistograms && ws->isHistogramData();
  }

  if (thereAreHistograms != allAreHistograms) {
    throw std::invalid_argument("Cannot mix histograms and point data.");
  }

  // All same size, same type - nothing to do
  if (minSize == maxSize) {
    return workspaces;
  }

  // Interpolate those that need interpolating
  std::vector<MatrixWorkspace_sptr> interpolatedWorkspaces;
  for (auto const &ws : workspaces) {
    if (ws->blocksize() < maxSize) {
      if (allAreHistograms) {
        interpolatedWorkspaces.push_back(
            interpolateHistogramWorkspace(ws, maxSize));
      } else {
        interpolatedWorkspaces.push_back(
            interpolatePointDataWorkspace(ws, maxSize));
      }
    } else {
      interpolatedWorkspaces.push_back(ws);
    }
  }

  return interpolatedWorkspaces;
}

MatrixWorkspace_sptr
JoinISISPolarizationEfficiencies::interpolatePointDataWorkspace(
    MatrixWorkspace_sptr ws, size_t const maxSize) {
  auto const &x = ws->x(0);
  auto const startX = x.front();
  auto const endX = x.back();
  Counts yVals(maxSize, 0.0);
  auto const dX = (endX - startX) / double(maxSize - 1);
  Points xVals(maxSize, LinearGenerator(startX, dX));
  auto newHisto = Histogram(xVals, yVals);
  interpolateLinearInplace(ws->histogram(0), newHisto);
  auto interpolatedWS = boost::make_shared<Workspace2D>();
  interpolatedWS->initialize(1, newHisto);
  assert(interpolatedWS->y(0).size() == maxSize);
  return interpolatedWS;
}

MatrixWorkspace_sptr
JoinISISPolarizationEfficiencies::interpolateHistogramWorkspace(
    MatrixWorkspace_sptr ws, size_t const maxSize) {
  ws->setDistribution(true);
  auto const &x = ws->x(0);
  auto const dX = (x.back() - x.front()) / double(maxSize);
  std::vector<double> params(2 * maxSize + 1);
  for (size_t i = 0; i < maxSize; ++i) {
    params[2 * i] = x.front() + dX * double(i);
    params[2 * i + 1] = dX;
  }
  params.back() = x.back();
  auto alg = createChildAlgorithm("InterpolatingRebin");
  alg->setProperty("InputWorkspace", ws);
  alg->setProperty("Params", params);
  alg->setProperty("OutputWorkspace", "dummy");
  alg->execute();
  MatrixWorkspace_sptr interpolatedWS = alg->getProperty("OutputWorkspace");
  assert(interpolatedWS->y(0).size() == maxSize);
  assert(interpolatedWS->x(0).size() == maxSize + 1);
  return interpolatedWS;
}

} // namespace DataHandling
} // namespace Mantid
