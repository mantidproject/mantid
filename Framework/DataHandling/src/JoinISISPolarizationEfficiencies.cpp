#include "MantidDataHandling/JoinISISPolarizationEfficiencies.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/make_unique.h"
#include <limits>

namespace Prop {

const std::string PP_WORKSPACE("Pp");
const std::string AP_WORKSPACE("Ap");
const std::string RHO_WORKSPACE("Rho");
const std::string ALPHA_WORKSPACE("Alpha");

const std::string P1_WORKSPACE("P1");
const std::string P2_WORKSPACE("P2");
const std::string F1_WORKSPACE("F1");
const std::string F2_WORKSPACE("F2");

const std::string OUTPUT_WORKSPACE("OutputWorkspace");

}

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(JoinISISPolarizationEfficiencies)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string JoinISISPolarizationEfficiencies::name() const {
  return "JoinISISPolarizationEfficiencies";
}

/// Algorithm's version for identification. @see Algorithm::version
int JoinISISPolarizationEfficiencies::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string JoinISISPolarizationEfficiencies::category() const {
  return "DataHandling\\Text;ILL\\Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string JoinISISPolarizationEfficiencies::summary() const {
  return "Joins workspaces containing ISIS reflectometry polarization "
         "efficiency factors from into a single workspace ready to be used in "
         "PolarizationEfficiencyCor.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void JoinISISPolarizationEfficiencies::init() {

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::PP_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the Pp polarization efficiency.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::AP_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the Ap polarization efficiency.");
  
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::RHO_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the Rho polarization efficiency.");
  
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::ALPHA_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the Alpha polarization efficiency.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::P1_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the P1 polarization efficiency.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::P2_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the P2 polarization efficiency.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::F1_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the F1 polarization efficiency.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          Prop::F2_WORKSPACE, "", Kernel::Direction::Input,
          PropertyMode::Optional),
      "A matrix workspaces containing the F2 polarization efficiency.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::OUTPUT_WORKSPACE, "", Direction::Output),
                  "An output workspace containing the efficiencies.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void JoinISISPolarizationEfficiencies::exec() {
  auto const propsFredrikze =
      getNonDefaultProperties({Prop::PP_WORKSPACE, Prop::AP_WORKSPACE,
                               Prop::RHO_WORKSPACE, Prop::ALPHA_WORKSPACE});
  auto const propsWildes =
      getNonDefaultProperties({Prop::P1_WORKSPACE, Prop::P2_WORKSPACE,
                               Prop::F1_WORKSPACE, Prop::F2_WORKSPACE});

  if (propsFredrikze.empty() && propsWildes.empty()) {
    throw std::invalid_argument("At least one of the efficiency file names must be set.");
  }

  if (!propsFredrikze.empty() && !propsWildes.empty()) {
    throw std::invalid_argument("Efficiencies belonging to different methods cannot mix.");
  }

  MatrixWorkspace_sptr efficiencies;
  if (!propsFredrikze.empty()) {
    efficiencies = createEfficiencies(propsFredrikze);
  } else {
    efficiencies = createEfficiencies(propsWildes);
  }

  setProperty("OutputWorkspace", efficiencies);
}

/// Get names of non-default properties out of a list of names
/// @param labels :: Names of properties to check.
std::vector<std::string>
JoinISISPolarizationEfficiencies::getNonDefaultProperties(
    std::vector<std::string> const &labels) const {
  std::vector<std::string> outputLabels;
  for (auto const &label : labels) {
    if (!isDefault(label)) {
      outputLabels.push_back(label);
    }
  }
  return outputLabels;
}

/// Load efficientcies from files and put them into a single workspace.
/// @param props :: Names of properties containg names of files to load.
MatrixWorkspace_sptr JoinISISPolarizationEfficiencies::createEfficiencies(
    std::vector<std::string> const &props) {
  std::vector<MatrixWorkspace_sptr> workspaces;
  for(auto const &propName : props) {
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
/// @param workspaces :: Workspaces to put together.
MatrixWorkspace_sptr JoinISISPolarizationEfficiencies::createEfficiencies(
    std::vector<std::string> const &labels,
    std::vector<MatrixWorkspace_sptr> const &workspaces) {
  auto rebinnedWorkspaces = rebinWorkspaces(workspaces);
  
  auto const &inWS = rebinnedWorkspaces.front();
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(inWS, labels.size(), inWS->x(0).size(), inWS->blocksize());
  auto axis1 = new TextAxis(labels.size());
  outWS->replaceAxis(1, axis1);

  for (size_t i = 0; i < rebinnedWorkspaces.size(); ++i) {
    auto &ws = rebinnedWorkspaces[i];
    outWS->mutableX(i) = ws->x(0);
    outWS->mutableY(i) = ws->y(0);
    outWS->mutableE(i) = ws->e(0);
    axis1->setLabel(i, labels[i]);
  }

  return outWS;
}

/// Rebin the workspaces so that all have the same blocksize.
/// @param workspaces :: The workspaces to rebin.
/// @return A list of rebinned workspaces.
std::vector<MatrixWorkspace_sptr>
JoinISISPolarizationEfficiencies::rebinWorkspaces(
    std::vector<MatrixWorkspace_sptr> const &workspaces) {
  size_t minSize(std::numeric_limits<size_t>::max());
  size_t maxSize(0);
  bool thereAreHistograms = false;
  bool allAreHistograms = true;

  // Find out if the workspaces need to be rebinned.
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

  // Rebin those that need rebinning
  std::vector<MatrixWorkspace_sptr> rebinnedWorkspaces;
  size_t const xSize = maxSize + (thereAreHistograms ? 1 : 0);
  for (auto const &ws : workspaces) {
    auto const &x = ws->x(0);
    if (x.size() < xSize) {
      auto const start = x.front();
      auto const end = x.back();
      auto const dx = (end - start) / double(maxSize);
      std::string const params = std::to_string(start) + "," + std::to_string(dx) + "," + std::to_string(end);
      auto rebin = createChildAlgorithm("Rebin");
      rebin->initialize();
      rebin->setProperty("InputWorkspace", ws);
      rebin->setProperty("Params", params);
      rebin->execute();
      MatrixWorkspace_sptr rebinnedWS = rebin->getProperty("OutputWorkspace");
      rebinnedWorkspaces.push_back(rebinnedWS);
      assert(rebinnedWS->x(0).size() == xSize);
    } else {
      rebinnedWorkspaces.push_back(ws);
    }
  }
  return rebinnedWorkspaces;
}

} // namespace DataHandling
} // namespace Mantid
