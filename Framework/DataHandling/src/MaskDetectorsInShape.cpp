#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidDataHandling/MaskSpectra.h"

#include "MantidAPI/Algorithm.tcc"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidIndexing/Conversion.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/IndexInfo.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(MaskDetectorsInShape)

using namespace Kernel;
using namespace API;

void MaskDetectorsInShape::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("Workspace", "", Direction::InOut),
      "The input workspace");
  declareProperty("ShapeXML", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The XML definition of the user defined shape.");
  declareProperty("IncludeMonitors", false, "Whether to include monitors if "
                                            "they are contained in the shape "
                                            "(default false)");
}

void MaskDetectorsInShape::exec() {
  // Get the input workspace
  MatrixWorkspace_sptr WS = getProperty("Workspace");

  const bool includeMonitors = getProperty("IncludeMonitors");
  const std::string shapeXML = getProperty("ShapeXML");

  std::vector<int> foundDets =
      runFindDetectorsInShape(WS, shapeXML, includeMonitors);
  if (foundDets.empty()) {
    g_log.information(
        "No detectors were found in the shape, nothing was masked");
    return;
  }
  runMaskDetectors(WS, foundDets);
  setProperty("Workspace", WS);
}

/// Run the FindDetectorsInShape Child Algorithm
std::vector<int> MaskDetectorsInShape::runFindDetectorsInShape(
    API::MatrixWorkspace_sptr workspace, const std::string shapeXML,
    const bool includeMonitors) {
  IAlgorithm_sptr alg = createChildAlgorithm("FindDetectorsInShape");
  alg->setPropertyValue("IncludeMonitors", includeMonitors ? "1" : "0");
  alg->setPropertyValue("ShapeXML", shapeXML);
  alg->setProperty<MatrixWorkspace_sptr>("Workspace", workspace);
  try {
    if (!alg->execute()) {
      throw std::runtime_error("FindDetectorsInShape Child Algorithm has not "
                               "executed successfully\n");
    }
  } catch (std::runtime_error &) {
    g_log.error(
        "Unable to successfully execute FindDetectorsInShape Child Algorithm");
    throw;
  }
  progress(0.5);

  // extract the results
  return alg->getProperty("DetectorList");
}

void MaskDetectorsInShape::runMaskDetectors(
    API::MatrixWorkspace_sptr workspace, const std::vector<int> &detectorIds) {
  auto alg = createChildAlgorithm("MaskSpectra", 0.85, 1.0);
  const auto &detInfo = workspace->detectorInfo();
  std::vector<size_t> detectorIndices;
  for (const auto id : detectorIds)
    detectorIndices.push_back(detInfo.indexOf(id));
  const auto &globalSpectrumIndices =
      workspace->indexInfo().globalSpectrumIndicesFromDetectorIndices(
          detectorIndices);
  alg->setWorkspaceInputProperties(
      "InputWorkspace", workspace, IndexType::WorkspaceIndex,
      Indexing::castVector<int64_t>(globalSpectrumIndices));
  alg->setProperty("OutputWorkspace", workspace);
  try {
    if (!alg->execute()) {
      throw std::runtime_error(
          "MaskDetectors Child Algorithm has not executed successfully\n");
    }
  } catch (std::runtime_error &) {
    g_log.error("Unable to successfully execute MaskDetectors Child Algorithm");
    throw;
  }
  progress(1);
}

} // namespace DataHandling
} // namespace Mantid
