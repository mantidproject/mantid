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
  return "DataHandling;ISIS\\Reflectometry";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadISISPolarizationEfficiencies::summary() const {
  return "Loads ISIS reflectometry polarization efficiency factors from files: "
         "one factor per file.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadISISPolarizationEfficiencies::init() {
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::PP_FILENAME, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the Pp polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::AP_FILENAME, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the Ap polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::RHO_FILENAME, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the Rho polarization efficiency "
                  "in XYE columns.");

  declareProperty(
      Kernel::make_unique<API::FileProperty>(Prop::ALPHA_FILENAME, "",
                                             API::FileProperty::OptionalLoad),
      "Path to the file containing the Alpha polarization efficiency "
      "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::P1_FILENAME, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the P1 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::P2_FILENAME, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the P2 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::F1_FILENAME, "", API::FileProperty::OptionalLoad),
                  "Path to the file containing the F1 polarization efficiency "
                  "in XYE columns.");

  declareProperty(Kernel::make_unique<API::FileProperty>(
                      Prop::F2_FILENAME, "", API::FileProperty::OptionalLoad),
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
    throw std::invalid_argument(
        "At least one of the efficiency file names must be set.");
  }

  if (!propsFredrikze.empty() && !propsWildes.empty()) {
    throw std::invalid_argument(
        "Efficiencies belonging to different methods cannot mix.");
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

  auto alg = createChildAlgorithm("JoinISISPolarizationEfficiencies");
  alg->initialize();
  for (auto const &propName : props) {
    auto loader = createChildAlgorithm("Load");
    loader->initialize();
    loader->setPropertyValue("Filename", getPropertyValue(propName));
    loader->execute();
    Workspace_sptr output = loader->getProperty("OutputWorkspace");
    auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    if (!ws) {
      throw std::invalid_argument("File " + propName +
                                  " cannot be loaded into a MatrixWorkspace.");
    }
    if (ws->getNumberHistograms() != 1) {
      throw std::runtime_error(
          "Loaded workspace must contain a single histogram. Found " +
          std::to_string(ws->getNumberHistograms()));
    }
    alg->setProperty(propName, ws);
  }
  alg->execute();
  MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  return outWS;
}

} // namespace DataHandling
} // namespace Mantid
