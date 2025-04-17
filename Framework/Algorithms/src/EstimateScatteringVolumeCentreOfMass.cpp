// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimateScatteringVolumeCentreOfMass.h"
#include "MantidAPI/InstrumentValidator.h"
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

EstimateScatteringVolumeCentreOfMass::EstimateScatteringVolumeCentreOfMass()
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr), m_elementPositions(), m_averagePos(), m_cubeSide(0.0) {}

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
  m_cubeSide = getProperty("ElementSize"); // in mm
  m_cubeSide *= 0.001;                     // now in m
  // Get a reference to the parameter map (used for indirect instruments)
  const ParameterMap &pmap = m_inputWS->constInstrumentParameters();

  // Construct Sample
  constructSample(m_inputWS->mutableSample());

  // Calculate the cached values of L1, element volumes, and geometry size
  auto integrationVolume = std::shared_ptr<const IObject>(m_sampleObject->clone());
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  }

  auto raster = Geometry::Rasterize::calculate(m_beamDirection, *integrationVolume, *m_sampleObject, m_cubeSide);
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_elementPositions = std::move(raster.position);
  if (m_elementPositions.empty()) {
    throw std::runtime_error("Failed to define any initial scattering gauge volume for geometry");
  }
  m_averagePos = calcAveragePosition(m_elementPositions);
  setProperty("CentreOfMass", m_averagePos);
}

/// Create the sample object using the Geometry classes, or use the existing one
void EstimateScatteringVolumeCentreOfMass::constructSample(API::Sample &sample) {
  m_sampleObject = &sample.getShape();
  // Check there is one, and fail if not
  if (!m_sampleObject->hasValidShape()) {
    const std::string mess("No shape has been defined for the sample in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  } else {
    g_log.information("Successfully constructed the sample object");
  }
}

std::shared_ptr<const Geometry::IObject> EstimateScatteringVolumeCentreOfMass::constructGaugeVolume() {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  std::shared_ptr<const Geometry::IObject> volume =
      ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
}

std::vector<double> EstimateScatteringVolumeCentreOfMass::calcAveragePosition(const std::vector<V3D> &pos) {
  V3D sum = std::accumulate(pos.begin(), pos.end(), V3D(0.0, 0.0, 0.0));

  if (!pos.empty()) {
    sum /= static_cast<double>(pos.size());
  }
  std::vector<double> out = {sum[0], sum[1], sum[2]};
  return out;
}

} // namespace Mantid::Algorithms
