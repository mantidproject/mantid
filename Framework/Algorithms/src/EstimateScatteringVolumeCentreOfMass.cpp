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
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
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

  // Retrieve the Sample Shape Geometry
  const Geometry::IObject_sptr sampleObject = extractValidSampleObject(m_inputWS->mutableSample());

  // Retrieve the illumination volume (we assume the sample is fully illuminated hence the volume is the same as the
  // sample, unless a Gauge Volume has been defined)
  //
  // NB: here we expect the gauge volume object to be the illumination volume in the lab frame, as such,
  // it is not required to be wholly within the sample shape (this will likely be the main use case for this alg)
  const Geometry::IObject_sptr integrationVolume =
      m_inputWS->run().hasProperty("GaugeVolume") ? getGaugeVolumeObject() : sampleObject;

  const V3D averagePos =
      rasterizeGaugeVolumeAndCalculateMeanElementPosition(beamDirection, integrationVolume, sampleObject);
  setProperty("CentreOfMass", std::vector<double>(averagePos));
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

const Geometry::IObject_sptr EstimateScatteringVolumeCentreOfMass::getGaugeVolumeObject() {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  const Geometry::IObject_sptr volume =
      ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
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
