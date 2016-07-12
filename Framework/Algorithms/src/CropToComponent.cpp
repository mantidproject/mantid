#include "MantidAlgorithms/CropToComponent.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/IDetector.h"

namespace {

void getDetectors(Mantid::API::MatrixWorkspace_sptr workspace, const std::vector<std::string>& componentNames, std::vector<Mantid::Geometry::IDetector_const_sptr>& detectors) {
  auto instrument = workspace->getInstrument();
  for (const auto &componentName : componentNames) {
      instrument->getDetectorsInBank(detectors, componentName);
  }
}

void getDetectorIDs(std::vector<Mantid::Geometry::IDetector_const_sptr>&detectors, std::vector<Mantid::detid_t>& detectorIDs){
  auto numberOfDetectors = detectors.size();
  for (size_t index = 0; index < numberOfDetectors; ++index) {
      auto det = detectors[index];
      detectorIDs[index] = det->getID();
  }
}
}


namespace Mantid
{
namespace Algorithms
{

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropToComponent)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CropToComponent::name() const { return "CropToComponent"; }

/// Algorithm's version for identification. @see Algorithm::version
int CropToComponent::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CropToComponent::category() const
{
    return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CropToComponent::summary() const
{
    return "Crops a workspace to a set of components.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CropToComponent::init()
{
    declareProperty(Kernel::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                        "InputWorkspace", "", Direction::Input),
                    "An input workspace.");
    declareProperty(Kernel::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
                        "OutputWorkspace", "", Direction::Output),
                    "An output workspace.");
    declareProperty(
        Kernel::make_unique<Mantid::Kernel::ArrayProperty<std::string>>(
            "ComponentNames"),
        "List of component names to which the workspace is goign to be cropped "
        "to.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CropToComponent::exec()
{
    // Get the names of the components
    std::vector<std::string> componentNames = getProperty("ComponentNames");
    Mantid::API::MatrixWorkspace_sptr inputWorkspace
        = getProperty("InputWorkspace");
    Mantid::API::MatrixWorkspace_sptr outputWorkspace
        = getProperty("OutputWorkspace");

    // Check if the component names are empty
    if (componentNames.empty()) {
        if (inputWorkspace == outputWorkspace) {
            setProperty("OutputWorkpspace", inputWorkspace);
        } else {
            auto cloned_uptr = inputWorkspace->clone();
            auto cloned_sptr
                = boost::make_shared<Mantid::API::MatrixWorkspace_sptr>(
                    std::move(cloned_uptr));
            setProperty("OutputWorkspace", cloned_sptr);
        }
        return;
    }

    // Get all detectors
    std::vector<Mantid::Geometry::IDetector_const_sptr> detectors;
    getDetectors(inputWorkspace, componentNames, detectors);

    // Get the detector IDs from the Detectors
    std::vector<detid_t> detectorIDs(detectors.size());
    getDetectorIDs(detectors, detectorIDs);

    // Run ExtractSpectra in order to obtain the cropped workspace
    auto extract_alg
        = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "ExtractSpectra");
    extract_alg->setChild(true);
    extract_alg->initialize();
    extract_alg->setProperty("InputWorkspace", inputWorkspace);
    extract_alg->setProperty("OutputWorkspace", outputWorkspace);
    extract_alg->setProperty("DetectorList", detectorIDs);
    extract_alg->execute();
    Mantid::API::MatrixWorkspace_sptr result
        = extract_alg->getProperty("OutputWorkspace");

    // Set the output
    setProperty("OutputWorkpspace", result);
}

} // namespace Algorithms
} // namespace Mantid
