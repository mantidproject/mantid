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
#include <algorithm>
#include <map>
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

std::map<std::string, std::string> EstimateScatteringVolumeCentreOfMass::validateInputs() {
  std::map<std::string, std::string> issues;

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    // absence is already reported by the property itself
    return issues;
  }

  // The element size is needed in metres for the gauge volume check below, so the units have to be
  // resolvable first
  const std::string elementUnits = getProperty("ElementUnits");
  const auto unitIt = unitToMeters.find(elementUnits);
  if (unitIt == unitToMeters.end()) {
    issues["ElementUnits"] = "Supported units for ElementUnits are (m, cm, mm), not: " + elementUnits;
    return issues;
  }
  const double elementSizeInUnits = getProperty("ElementSize");
  const double elementSize = elementSizeInUnits * unitIt->second;

  const auto &sample = inputWS->sample();
  if (!sample.getShape().hasValidShape()) {
    issues["InputWorkspace"] = "No shape has been defined for the sample in the input workspace";
  }

  // A gauge volume must be resolvable into a shape, and has to be big enough to hold at least one
  // integration element in every dimension or the raster below produces nothing.
  const auto &run = inputWS->run();
  if (run.hasProperty("GaugeVolume")) {
    Geometry::IObject_sptr gauge;
    try {
      gauge = Geometry::ShapeFactory().createShape(run.getProperty("GaugeVolume")->value());
    } catch (const std::exception &e) {
      issues["InputWorkspace"] = std::string("Could not create a shape from the GaugeVolume sample log: ") + e.what();
    }
    if (gauge && gauge->hasValidShape()) {
      const auto bbox = gauge->getBoundingBox();
      if ((bbox.xMax() - bbox.xMin()) < elementSize || (bbox.yMax() - bbox.yMin()) < elementSize ||
          (bbox.zMax() - bbox.zMin()) < elementSize) {
        issues["ElementSize"] = "The gauge volume is smaller than a single integration element in at least one "
                                "dimension - reduce the ElementSize";
      }
    } else if (!gauge) {
      issues["InputWorkspace"] = "The GaugeVolume sample log does not describe a valid shape";
    }
  }

  return issues;
}

void EstimateScatteringVolumeCentreOfMass::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Cache the beam direction
  const V3D beamDirection = m_inputWS->getInstrument()->getBeamDirection();
  // Calculate the element size. The units are checked in validateInputs.
  m_cubeSide = getProperty("ElementSize"); // in units
  const std::string elementUnits = getProperty("ElementUnits");
  m_cubeSide *= unitToMeters.at(elementUnits); // now in m

  const Geometry::IObject_sptr sampleObject = extractValidSampleObject(m_inputWS->mutableSample());
  const Kernel::Matrix<double> gonioR = outstandingSampleRotation(*sampleObject);

  const auto raster = rasterizeScatteringVolume(*sampleObject, gonioR, beamDirection);
  if (raster.position.empty()) {
    // most errors should be caught by the rasterise function, but just in case
    const std::string mess("Failed to find any points in the rasterized illumination volume within the sample shape - "
                           "Check sample shape and gauge volume are defined correctly or try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }

  const V3D centre = calcVolumeWeightedCentre(raster);
  // A gauge volume is authored in the lab frame, so calculateInLabFrame already reports positions
  // there. Without one the sample was rasterised in its own frame, so the centre still has to be
  // rotated out. Rotating the centre rather than every element is equivalent, since the goniometer
  // rotation is linear and so commutes with the weighted mean.
  const bool rasterIsAlreadyInLabFrame = m_inputWS->run().hasProperty("GaugeVolume");
  setProperty("CentreOfMass", std::vector<double>(rasterIsAlreadyInLabFrame ? centre : gonioR * centre));
}

/// How much of the workspace's goniometer rotation still has to be applied to reach the lab frame.
///
/// A workspace can arrive here with its sample shape in either frame. CopySample bakes the
/// destination's goniometer into the shape definition, so the shape is already in the lab frame,
/// while SetGoniometer on its own leaves the shape untouched in its own frame. Both leave the same
/// goniometer on the run, so the run alone cannot distinguish them - applying R unconditionally
/// rotates an already-rotated shape a second time.
///
/// Asking the shape what it has already been rotated by resolves it: with B baked in and the run
/// reporting R, what remains is R*B^-1. That is the identity when the shape is already in the lab
/// frame, and R when it is not, without the caller having to know which route built the workspace.
const Kernel::Matrix<double>
EstimateScatteringVolumeCentreOfMass::outstandingSampleRotation(const Geometry::IObject &sampleObject) const {
  const auto gonioR = m_inputWS->run().getGoniometer().getR();
  Kernel::Matrix<double> alreadyApplied = sampleObject.getAppliedRotation();
  if (alreadyApplied == Kernel::Matrix<double>(3, 3, true)) {
    return gonioR;
  }
  g_log.information("The sample shape already carries a rotation, so only the remainder of the "
                    "goniometer rotation is applied");
  alreadyApplied.Invert();
  return gonioR * alreadyApplied;
}

/// The sample shape is expressed in whichever frame it arrived in; outstandingSampleRotation works
/// out how much of the goniometer rotation is left to apply, and that is the gonioR passed here.
/// The gauge volume, if any, is defined in the lab frame, so the two need reconciling before they
/// can be intersected - which is what Rasterize::calculateInLabFrame does.
Geometry::Raster EstimateScatteringVolumeCentreOfMass::rasterizeScatteringVolume(const Geometry::IObject &sampleObject,
                                                                                 const Kernel::Matrix<double> &gonioR,
                                                                                 const V3D &beamDirection) {
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    g_log.information("Calculating scattering within the gauge volume defined on the input workspace");
    // validateInputs has already confirmed this builds a valid shape spanning at least one element
    const Geometry::IObject_sptr gauge =
        Geometry::ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());
    return Geometry::Rasterize::calculateInLabFrame(beamDirection, *gauge, sampleObject, m_cubeSide, gonioR);
  }
  // With no gauge volume the illuminated volume is the whole sample, so both shapes share the
  // sample's own frame and the ordinary raster applies.
  return Geometry::Rasterize::calculate(beamDirection, sampleObject, sampleObject, m_cubeSide);
}

/// Create the sample object using the Geometry classes, or use the existing one.
/// The shape is validated in validateInputs.
const Geometry::IObject_sptr EstimateScatteringVolumeCentreOfMass::extractValidSampleObject(const API::Sample &sample) {
  return sample.getShapePtr();
}

V3D EstimateScatteringVolumeCentreOfMass::calcVolumeWeightedCentre(const Geometry::Raster &raster) {
  V3D weightedSum(0.0, 0.0, 0.0);
  double totalVolume = 0.0;
  for (size_t i = 0; i < raster.position.size(); ++i) {
    weightedSum += raster.position[i] * raster.volume[i];
    totalVolume += raster.volume[i];
  }
  return weightedSum / totalVolume;
}

} // namespace Mantid::Algorithms
