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
#include "MantidKernel/V3D.h"

namespace Mantid::Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EstimateScatteringVolumeCentreOfMass)

EstimateScatteringVolumeCentreOfMass::EstimateScatteringVolumeCentreOfMass()
    : API::Algorithm(), m_inputWS(), m_cubeSide(0.0) {}

void EstimateScatteringVolumeCentreOfMass::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "Input Workspace");
  declareProperty(std::make_unique<PropertyWithValue<std::vector<double>>>("CentreOfMass", V3D(), Direction::Output),
                  "Estimated centre of mass of illuminated sample volume");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, "The size of one side of an integration element cube in mm");
}

void EstimateScatteringVolumeCentreOfMass::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Cache the beam direction
  m_beamDirection = m_inputWS->getInstrument()->getBeamDirection();
  // Calculate the element size
  m_cubeSide = getProperty("ElementSize"); // in mm
  m_cubeSide *= 0.001;                     // now in m

  // Retrieve the Sample Shape Geometry
  const std::shared_ptr<const Geometry::IObject> sampleObject = extractValidSampleObject(m_inputWS->mutableSample());

  // Retrieve the illumination volume (we assume the sample is fully illuminated hence the volume is the same as the
  // sample, unless a Gauge Volume has been defined)
  //
  // NB: here we expect the gauge volume object to be the illumination volume in the lab frame, as such,
  // it is not required to be wholly within the sample shape (this will likely be the main use case for this alg)
  std::shared_ptr<const IObject> integrationVolume = sampleObject;
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = getGaugeVolumeObject();
  }
  const V3D averagePos = rasterizeGaugeVolumeAndCalculateMeanElementPosition(integrationVolume, sampleObject);
  setProperty("CentreOfMass", averagePos);
}

/// Calculate as raster of the illumination volume, evaluating which points are within the sample geometry.
/// Calculate the mean position of all valid volume elements to get the Centre of Mass of the Scattering Volume
const V3D EstimateScatteringVolumeCentreOfMass::rasterizeGaugeVolumeAndCalculateMeanElementPosition(
    const std::shared_ptr<const IObject> integrationVolume, const std::shared_ptr<const IObject> sampleObject) {
  const auto raster = Geometry::Rasterize::calculate(m_beamDirection, *integrationVolume, *sampleObject, m_cubeSide);
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // calculate mean element position
  const V3D meanPos = calcAveragePosition(raster.position);
  return meanPos;
}

/// Create the sample object using the Geometry classes, or use the existing one
const std::shared_ptr<const Geometry::IObject>
EstimateScatteringVolumeCentreOfMass::extractValidSampleObject(const API::Sample &sample) {
  // Check there is one, and fail if not
  if (!sample.getShape().hasValidShape()) {
    const std::string mess("No shape has been defined for the sample in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  } else {
    g_log.information("Successfully constructed the sample object");
    return std::make_shared<const Geometry::IObject>(sample.getShape());
  }
}

const std::shared_ptr<const Geometry::IObject> EstimateScatteringVolumeCentreOfMass::getGaugeVolumeObject() {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  const std::shared_ptr<const Geometry::IObject> volume =
      ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
}

const V3D EstimateScatteringVolumeCentreOfMass::calcAveragePosition(const std::vector<V3D> &pos) {
  if (!pos.empty()) {
    V3D sum = std::accumulate(pos.begin(), pos.end(), V3D(0.0, 0.0, 0.0));
    sum /= static_cast<double>(pos.size());
    return sum;
  } else {
  }
  const std::string mess("No intersection points found between illumination volume and sample shape - "
                         "Check sample shape and gauge volume are defined correctly or try reducing the ElementSize");
  g_log.error(mess);
  throw std::runtime_error(mess);
}

} // namespace Mantid::Algorithms
