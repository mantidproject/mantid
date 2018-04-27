#include "MantidDataHandling/LoadISISPolarizationEfficiencies.h"

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

const std::string PP_FILENAME("Pp");
const std::string AP_FILENAME("Ap");
const std::string RHO_FILENAME("Rho");
const std::string ALPHA_FILENAME("Alpha");

const std::string P1_FILENAME("P1");
const std::string P2_FILENAME("P2");
const std::string F1_FILENAME("F1");
const std::string F2_FILENAME("F2");

const std::string OUTPUT_WORKSPACE("OutputWorkspace");

}

namespace Mantid {
namespace DataHandling {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadISISPolarizationEfficiencies)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadISISPolarizationEfficiencies::name() const {
  return "LoadISISPolarizationEfficiencies";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadISISPolarizationEfficiencies::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadISISPolarizationEfficiencies::category() const {
  return "DataHandling\\Text;ILL\\Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadISISPolarizationEfficiencies::summary() const {
  return "Loads ISIS reflectometry polarization efficiency factors from files: one factor per file.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadISISPolarizationEfficiencies::init() {
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::PP_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the Pp polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::AP_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the Ap polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::RHO_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the Rho polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::ALPHA_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the Alpha polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::P1_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the P1 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::P2_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the P2 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::F1_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the F1 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::F2_FILENAME, "", API::FileProperty::Load),
                  "Path to the file containing the F2 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      Prop::OUTPUT_WORKSPACE, "", Direction::Output),
                  "An output workspace containing the efficiencies.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadISISPolarizationEfficiencies::exec() {
  auto const propsFredrikze =
      getNonDefaultProperties({Prop::PP_FILENAME, Prop::AP_FILENAME,
                               Prop::RHO_FILENAME, Prop::ALPHA_FILENAME});
  auto const propsWildes =
      getNonDefaultProperties({Prop::P1_FILENAME, Prop::P2_FILENAME,
                               Prop::F1_FILENAME, Prop::F2_FILENAME});

  if (propsFredrikze.empty() && propsWildes.empty()) {
    throw std::invalid_argument("At least one of the efficiency file names must be set.");
  }

  if (!propsFredrikze.empty() && !propsWildes.empty()) {
    throw std::invalid_argument("Efficiencies belonging to different methods cannot mix.");
  }

  MatrixWorkspace_sptr efficiencies;
  if (!propsFredrikze.empty()) {
    efficiencies = loadEfficiencies(propsFredrikze);
  } else {
    efficiencies = loadEfficiencies(propsWildes);
  }

  setProperty("OutputWorkspace", efficiencies);
}

/// Get names of non-default properties out of a list of names
/// @param labels :: Names of properties to check.
std::vector<std::string>
LoadISISPolarizationEfficiencies::getNonDefaultProperties(
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
MatrixWorkspace_sptr LoadISISPolarizationEfficiencies::loadEfficiencies(
    std::vector<std::string> const &props) {
  std::vector<MatrixWorkspace_sptr> workspaces;
  for(auto const &propName : props) {
    auto alg = createChildAlgorithm("Load");
    alg->initialize();
    alg->setPropertyValue("Filename", getPropertyValue(propName));
    alg->execute();
    MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
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
MatrixWorkspace_sptr LoadISISPolarizationEfficiencies::createEfficiencies(
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
LoadISISPolarizationEfficiencies::rebinWorkspaces(
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
  
  // All same size, same type - nothing to do
  if (minSize == maxSize && thereAreHistograms == allAreHistograms) {
    return workspaces;
  }

  // Rebin those that need rebinning
  std::vector<MatrixWorkspace_sptr> rebinnedWorkspaces;
  size_t xSize = maxSize + (thereAreHistograms ? 1 : 0);
  for (auto const &ws : workspaces) {
    auto const &x = ws->x(0);
    if (x.size() < xSize) {
      auto const start = x.front();
      auto const end = x.back();
      auto const dx = (end - start) / double(xSize - 1);
      std::string const params = std::to_string(start) + "," + std::to_string(dx) + "," + std::to_string(end);
      auto rebin = createChildAlgorithm("Rebin");
      rebin->initialize();
      rebin->setProperty("InputWorkspace", ws);
      rebin->setProperty("Params", params);
      rebin->execute();
      MatrixWorkspace_sptr rebinnedWS = rebin->getProperty("OutputWorkspace");
      rebinnedWorkspaces.push_back(rebinnedWS);
    } else {
      rebinnedWorkspaces.push_back(ws);
    }
  }
  return rebinnedWorkspaces;
}

} // namespace DataHandling
} // namespace Mantid
