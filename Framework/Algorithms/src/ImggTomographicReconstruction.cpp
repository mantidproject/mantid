#include "MantidAlgorithms/ImggTomographicReconstruction.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImggTomographicReconstruction)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ImggTomographicReconstruction::name() const {
  return "ImggTomographicReconstruction";
}

/// Algorithm's version for identification. @see Algorithm::version
int ImggTomographicReconstruction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImggTomographicReconstruction::category() const {
  return "Algorithms\\Tomography";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ImggTomographicReconstruction::summary() const {
  return "Reconstruct a 3D volume from 2D imaging projection data";
}

namespace {
const std::string PROP_INPUT_WS = "InputWorkspace";
const std::string PROP_METHOD = "Method";
const std::string PROP_OUTPUT_WS = "OutputWorkspace";
const std::string PROP_RELAXATION_PARAM = "RelaxationParameter";
const std::string PROP_MAX_CORES = "MaximumCores";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImggTomographicReconstruction::init() {
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<>>(
          PROP_INPUT_WS, "", Kernel::Direction::Input,
          boost::make_shared<API::WorkspaceUnitValidator>("Label")),
      "Workspace holding an image (with one spectrum per pixel row).");

  std::vector<std::string> methods{"FBP (tomopy)"};
  declareProperty(
      PROP_METHOD, methods.front(),
      boost::make_shared<Kernel::ListValidator<std::string>>(methods),
      "Reconstruction method", Kernel::Direction::Input);

  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
      PROP_OUTPUT_WS, "", Kernel::Direction::Output));

  auto zeroOrPosDbl = boost::make_shared<Kernel::BoundedValidator<double>>();
  zeroOrPosDbl->setLower(0.0);
  declareProperty(PROP_RELAXATION_PARAM, 0.5, zeroOrPosDbl,
                  "Relaxation parameter for the reconstruction method.");

  auto zeroOrPosInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  zeroOrPosDbl->setLower(0);
  declareProperty(PROP_MAX_CORES, 0, zeroOrPosDbl,
                  "Maximum number of cores to use for parallel runs. Leave "
                  "empty to use all available cores.");
}

std::map<std::string, std::string>
ImggTomographicReconstruction::validateInputs() {
  std::map<std::string, std::string> result;

  result[PROP_INPUT_WS] =
      "More options required. This work in progress and unstable";

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ImggTomographicReconstruction::exec() {
  API::WorkspaceGroup_sptr ws = getProperty(PROP_INPUT_WS);

  g_log.notice() << "Reconstructing volume from workspace " << ws->getTitle()
                 << std::endl;
}

} // namespace Algorithms
} // namespace Mantid
