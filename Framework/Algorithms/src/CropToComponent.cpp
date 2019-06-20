// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CropToComponent.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidIndexing/Conversion.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"

namespace {
std::vector<size_t>
getDetectorIndices(const Mantid::API::MatrixWorkspace &workspace,
                   const std::vector<std::string> &componentNames) {
  const auto &compInfo = workspace.componentInfo();
  const auto instrument = workspace.getInstrument();
  std::vector<size_t> detIndices;
  for (const auto &componentName : componentNames) {
    const auto comp = instrument->getComponentByName(componentName);
    const auto compIndex = compInfo.indexOf(comp->getComponentID());
    const auto indices = compInfo.detectorsInSubtree(compIndex);
    detIndices.insert(detIndices.end(), indices.begin(), indices.end());
  }
  return detIndices;
}
} // namespace

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CropToComponent)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CropToComponent::name() const { return "CropToComponent"; }

/// Algorithm's version for identification. @see Algorithm::version
int CropToComponent::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CropToComponent::category() const {
  return "Transforms\\Splitting";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CropToComponent::summary() const {
  return "Crops a workspace to a set of components.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CropToComponent::init() {
  declareProperty(
      std::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      std::make_unique<WorkspaceProperty<Mantid::API::MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "An output workspace.");
  declareProperty(
      std::make_unique<Mantid::Kernel::ArrayProperty<std::string>>(
          "ComponentNames"),
      "List of component names which are used to crop the workspace."
      "to.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CropToComponent::exec() {
  // Get the names of the components
  std::vector<std::string> componentNames = getProperty("ComponentNames");
  Mantid::API::MatrixWorkspace_sptr inputWorkspace =
      getProperty("InputWorkspace");

  // Get all detectors
  const auto &detectorIndices =
      getDetectorIndices(*inputWorkspace, componentNames);

  // Get the corresponding workspace indices from the detectors
  const auto &workspaceIndices =
      inputWorkspace->indexInfo().globalSpectrumIndicesFromDetectorIndices(
          detectorIndices);

  // Run ExtractSpectra in order to obtain the cropped workspace
  auto extract_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "ExtractSpectra");
  extract_alg->setChild(true);
  extract_alg->initialize();
  extract_alg->setProperty("InputWorkspace", inputWorkspace);
  extract_alg->setProperty("OutputWorkspace", "dummy");
  extract_alg->setProperty("WorkspaceIndexList",
                           Indexing::castVector<size_t>(workspaceIndices));
  extract_alg->execute();
  Mantid::API::MatrixWorkspace_sptr outputWorkspace =
      extract_alg->getProperty("OutputWorkspace");

  // Set the output
  setProperty("OutputWorkspace", outputWorkspace);
}

std::map<std::string, std::string> CropToComponent::validateInputs() {
  std::map<std::string, std::string> result;
  Mantid::API::MatrixWorkspace_sptr inputWorkspace =
      getProperty("InputWorkspace");
  std::vector<std::string> componentNames = getProperty("ComponentNames");

  // Make sure that the component exists on the input workspace
  auto instrument = inputWorkspace->getInstrument();
  for (auto &componentName : componentNames) {
    auto detector = instrument->getComponentByName(componentName);
    if (!detector) {
      std::string message =
          "The component name " + componentName +
          " does not exist on the workspace. Specify a valid component.";
      result["ComponentNames"] = message;
      break;
    }
  }
  return result;
}

} // namespace Algorithms
} // namespace Mantid
