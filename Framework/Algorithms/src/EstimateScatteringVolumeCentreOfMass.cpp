// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimateScatteringVolumeCentreOfMass.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <unordered_map>

namespace Mantid::Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;

namespace {
const std::string UNIT_M = "m";
const std::string UNIT_CM = "cm";
const std::string UNIT_MM = "mm";
static const std::unordered_map<std::string, double> unitToMeters{{UNIT_M, 1.0}, {UNIT_CM, 0.01}, {UNIT_MM, 0.001}};
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EstimateScatteringVolumeCentreOfMass)

EstimateScatteringVolumeCentreOfMass::EstimateScatteringVolumeCentreOfMass()
    : API::Algorithm(), m_inputWS(), m_cubeSide(0.0) {}

void EstimateScatteringVolumeCentreOfMass::init() {

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<InstrumentValidator>()),
                  "Input Workspace");
  declareProperty(std::make_unique<PropertyWithValue<std::vector<double>>>("CentreOfMass", V3D(), Direction::Output),
                  "Estimated centre of mass of illuminated sample volume");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(1e-6);
  declareProperty("ElementSize", 1.0, moreThanZero,
                  "The size of one side of an integration element cube in {ElementUnits}");

  declareProperty("ElementUnits", UNIT_MM,
                  std::make_shared<StringListValidator>(std::vector<std::string>{UNIT_M, UNIT_CM, UNIT_MM}),
                  "The units which ElementSize has been provided in");
}

void EstimateScatteringVolumeCentreOfMass::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Cache the beam direction
  const V3D beamDirection = m_inputWS->getInstrument()->getBeamDirection();
  // Calculate the element size
  m_cubeSide = getProperty("ElementSize"); // in units
  const std::string elementUnits = getProperty("ElementUnits");

  auto it = unitToMeters.find(elementUnits);
  if (it == unitToMeters.end()) {
    throw std::invalid_argument("Supported units for ElementUnits are (m, cm, mm), not: " + elementUnits);
  }
  m_cubeSide *= it->second; // now in m

  // The sample shape on the workspace already has any initial rotation baked into its definition,
  // so it is expressed in the sample shape's own frame. The workspace's goniometer R describes
  // the additional rotation from that frame into the lab frame. The gauge volume (if any) is
  // defined in the lab frame.
  //
  // When a gauge volume is present we rasterise it in the lab frame and transform each candidate
  // voxel into the sample shape's frame via R.inv() to test inclusion against the sample. Doing
  // the intersection this way - rather than rotating the gauge into the sample frame - keeps the
  // gauge's axis-aligned bounding box tight even for non-axis-aligned rotations; rotating the
  // gauge would inflate its bbox and silently admit voxels outside the actual gauge volume.
  //
  // With no gauge volume the illumination volume equals the sample, so we rasterise the sample
  // in its own frame (where the rasterise loop only ever accepts points inside the sample anyway)
  // and rotate the resulting mean position into the lab frame.
  const Geometry::IObject_sptr sampleObject = extractValidSampleObject(m_inputWS->mutableSample());
  const Kernel::Matrix<double> gonioR = m_inputWS->run().getGoniometer().getR();

  V3D averagePosInLabFrame;
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    averagePosInLabFrame = rasterizeLabGaugeAndCalculateMeanElementPosition(*sampleObject, gonioR);
  } else {
    const V3D averagePosInShapeFrame =
        rasterizeGaugeVolumeAndCalculateMeanElementPosition(beamDirection, sampleObject, sampleObject);
    averagePosInLabFrame = gonioR * averagePosInShapeFrame;
  }
  setProperty("CentreOfMass", std::vector<double>(averagePosInLabFrame));
}

/// Calculate as raster of the illumination volume, evaluating which points are within the sample geometry.
/// Calculate the mean position of all valid volume elements to get the Centre of Mass of the Scattering Volume
const V3D EstimateScatteringVolumeCentreOfMass::rasterizeGaugeVolumeAndCalculateMeanElementPosition(
    const V3D beamDirection, const Geometry::IObject_sptr integrationVolume,
    const Geometry::IObject_sptr sampleObject) {
  const auto raster = Geometry::Rasterize::calculate(beamDirection, *integrationVolume, *sampleObject, m_cubeSide);
  if (raster.l1.size() == 0) {
    // most errors should be caught by the rasterise function, but just in case
    const std::string mess("Failed to find any points in the rasterized illumination volume within the sample shape - "
                           "Check sample shape and gauge volume are defined correctly or try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
  // calculate mean element position
  const V3D meanPos = calcAveragePosition(raster.position);
  return meanPos;
}

/// Create the sample object using the Geometry classes, or use the existing one
const Geometry::IObject_sptr EstimateScatteringVolumeCentreOfMass::extractValidSampleObject(const API::Sample &sample) {
  // Check there is one, and fail if not
  if (!sample.getShape().hasValidShape()) {
    const std::string mess("No shape has been defined for the sample in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  } else {
    g_log.information("Successfully extracted the sample object");
    return sample.getShapePtr();
  }
}

const V3D EstimateScatteringVolumeCentreOfMass::rasterizeLabGaugeAndCalculateMeanElementPosition(
    const Geometry::IObject &sampleObject, const Kernel::Matrix<double> &gonioR) {
  g_log.information("Calculating scattering within the gauge volume defined on the input workspace");
  const std::string xml = m_inputWS->run().getProperty("GaugeVolume")->value();
  const Geometry::IObject_sptr gauge = Geometry::ShapeFactory().createShape(xml);

  Kernel::Matrix<double> gonioRInv(gonioR);
  gonioRInv.Invert();
  const bool gonioIsIdentity = (gonioR == Kernel::Matrix<double>(3, 3, true));

  const auto bbox = gauge->getBoundingBox();
  const double xLength = bbox.xMax() - bbox.xMin();
  const double yLength = bbox.yMax() - bbox.yMin();
  const double zLength = bbox.zMax() - bbox.zMin();
  const auto numXSlices = static_cast<size_t>(xLength / m_cubeSide);
  const auto numYSlices = static_cast<size_t>(yLength / m_cubeSide);
  const auto numZSlices = static_cast<size_t>(zLength / m_cubeSide);
  if (numXSlices == 0 || numYSlices == 0 || numZSlices == 0) {
    const std::string mess("Gauge volume is too small for the chosen ElementSize - "
                           "try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
  const double dx = xLength / static_cast<double>(numXSlices);
  const double dy = yLength / static_cast<double>(numYSlices);
  const double dz = zLength / static_cast<double>(numZSlices);

  V3D sum(0.0, 0.0, 0.0);
  size_t count = 0;
  for (size_t i = 0; i < numZSlices; ++i) {
    const double z = (static_cast<double>(i) + 0.5) * dz + bbox.zMin();
    for (size_t j = 0; j < numYSlices; ++j) {
      const double y = (static_cast<double>(j) + 0.5) * dy + bbox.yMin();
      for (size_t k = 0; k < numXSlices; ++k) {
        const double x = (static_cast<double>(k) + 0.5) * dx + bbox.xMin();
        const V3D pLab(x, y, z);
        // Reject voxels outside the actual (lab-frame) gauge volume. For an axis-aligned gauge
        // the bbox is tight and this is a no-op, but for any non-axis-aligned authored gauge
        // it correctly clips the iteration to the gauge interior.
        if (!gauge->isValid(pLab)) {
          continue;
        }
        // Test inclusion against the sample shape in its own frame.
        const V3D pShape = gonioIsIdentity ? pLab : gonioRInv * pLab;
        if (!sampleObject.isValid(pShape)) {
          continue;
        }
        sum += pLab;
        ++count;
      }
    }
  }
  if (count == 0) {
    const std::string mess("Failed to find any voxels inside both the gauge volume and the sample "
                           "shape - check sample shape and gauge volume are defined correctly or "
                           "try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
  sum /= static_cast<double>(count);
  return sum;
}

const V3D EstimateScatteringVolumeCentreOfMass::calcAveragePosition(const std::vector<V3D> &pos) {
  if (!pos.empty()) {
    V3D sum = std::accumulate(pos.begin(), pos.end(), V3D(0.0, 0.0, 0.0));
    sum /= static_cast<double>(pos.size());
    return sum;
  } else {
    // shouldn't be able to reach this point anyway
    const std::string mess("No intersection points found between illumination volume and sample shape - "
                           "Check sample shape and gauge volume are defined correctly or try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
}

} // namespace Mantid::Algorithms
