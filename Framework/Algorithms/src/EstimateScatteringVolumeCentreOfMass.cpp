// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EstimateScatteringVolumeCentreOfMass.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/RadialCollimatorProfile.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include <algorithm>
#include <cmath>
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
/// Instrument parameter holding the calibrated collimator gauge width, in metres. Written by the
/// Engineering Diffraction correction workflow from the known experimental configuration.
const std::string COL_GAUGE_WIDTH_PARAM = "col-gauge-width";
/// Upper bound on the number of wavelength points summed over when weighting by attenuation.
constexpr size_t MAX_WAVELENGTH_SAMPLES = 50;
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

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("DetectorScatteringCentres", "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "TableWorkspace containing estimated neutron weighted scattering centres per spectra.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("UseNeutronWeightings", false, Direction::Input),
                  "Whether the full, per-spectrum neutron weighted centre of mass should be calculated or just the "
                  "geometric centre of mass");

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

  const bool useNeutronWeightings = getProperty("UseNeutronWeightings");
  if (useNeutronWeightings) {
    // Weighting the scattering points by attenuation is meaningless without something to attenuate in.
    if (sample.getMaterial().numberDensity() <= 0.0) {
      issues["UseNeutronWeightings"] = "Neutron weighting requires a sample material - set one with SetSampleMaterial, "
                                       "or leave UseNeutronWeightings off for a purely geometric centre of mass";
    }
    // The attenuation is summed over the workspace's wavelength bins, so they have to be wavelengths.
    const auto &xUnit = inputWS->getAxis(0)->unit();
    if (!xUnit || xUnit->unitID() != "Wavelength") {
      issues["InputWorkspace"] = "Neutron weighting sums attenuation over the workspace's wavelength bins, so the "
                                 "InputWorkspace must be in units of Wavelength - run ConvertUnits first";
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

  m_doWeighted = getProperty("UseNeutronWeightings");

  const Geometry::IObject_sptr sampleObject = extractValidSampleObject(m_inputWS->mutableSample());
  const Kernel::Matrix<double> gonioR = m_inputWS->run().getGoniometer().getR();

  const auto elements = generateScatteringVolumeElements(*sampleObject, gonioR, beamDirection);

  if (!m_doWeighted) {
    const V3D averagePosInLabFrame = calcAveragePosition(elements);
    setProperty("CentreOfMass", std::vector<double>(averagePosInLabFrame));
    return;
  }

  std::vector<V3D> centres;
  std::vector<double> weights;
  calcDetectorScatteringCentres(*sampleObject, gonioR, elements, centres, weights);

  // The centre of mass for the experiment as a whole is the mean of the per-detector centres weighted
  // by how much each detector actually sees, mirroring the intensity weighting of eq. 20 of
  // Creek, Santisteban & Edwards (2005).
  V3D weightedSum(0.0, 0.0, 0.0);
  double totalWeight = 0.0;
  auto table = WorkspaceFactory::Instance().createTable();
  table->addColumn("int", "detid");
  table->addColumn("double", "x");
  table->addColumn("double", "y");
  table->addColumn("double", "z");
  table->addColumn("double", "weight");

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  size_t unseenCount = 0;
  for (size_t i = 0; i < weights.size(); ++i) {
    if (weights[i] <= 0.0) {
      if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i) && !spectrumInfo.isMasked(i)) {
        ++unseenCount;
      }
      continue;
    }
    weightedSum += centres[i] * weights[i];
    totalWeight += weights[i];
    for (const auto detid : m_inputWS->getSpectrum(i).getDetectorIDs()) {
      TableRow row = table->appendRow();
      row << static_cast<int>(detid) << centres[i].X() << centres[i].Y() << centres[i].Z() << weights[i];
    }
  }
  if (unseenCount > 0) {
    g_log.warning("No part of the scattering volume is visible to " + std::to_string(unseenCount) +
                  " detectors - they have been left out of the scattering centre table");
  }
  if (totalWeight <= 0.0) {
    const std::string mess("No detector sees any part of the scattering volume - check the sample shape, gauge "
                           "volume, beam and collimator definitions");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }

  setProperty("CentreOfMass", std::vector<double>(weightedSum / totalWeight));
  if (!getPropertyValue("DetectorScatteringCentres").empty()) {
    setProperty("DetectorScatteringCentres", table);
  }
}

/// Weight each integration element by the spatial resolution function SRF(r) = P_i(r) P_s(r) P_d(r)
/// and reduce to one centre of mass per detector.
///
/// P_i, the incident beam profile, and the incoming attenuation path are detector independent, so
/// they are evaluated once per element. P_d, the collimator acceptance, and the outgoing attenuation
/// path depend on the detector and are evaluated in the per-detector loop - the outgoing path is the
/// only genuinely per-detector physical effect here, and is what makes the centres differ between
/// detectors.
void EstimateScatteringVolumeCentreOfMass::calcDetectorScatteringCentres(const Geometry::IObject &sampleObject,
                                                                         const Kernel::Matrix<double> &gonioR,
                                                                         const std::vector<V3D> &elements,
                                                                         std::vector<V3D> &centres,
                                                                         std::vector<double> &weights) {
  const auto instrument = m_inputWS->getInstrument();
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const auto nHist = m_inputWS->getNumberHistograms();

  Kernel::Matrix<double> gonioRInv(gonioR);
  gonioRInv.Invert();
  const bool gonioIsIdentity = (gonioR == Kernel::Matrix<double>(3, 3, true));
  const auto toShapeFrame = [&](const V3D &pLab) { return gonioIsIdentity ? pLab : gonioRInv * pLab; };

  // P_i - the beam profile knows nothing about the goniometer, so it is evaluated in the lab frame.
  const auto beamProfile = BeamProfileFactory::createBeamProfile(*instrument, m_inputWS->sample());

  // P_d - only applied when the collimator has been calibrated onto the workspace.
  std::unique_ptr<RadialCollimatorProfile> collimator;
  if (instrument->hasParameter(COL_GAUGE_WIDTH_PARAM)) {
    const auto widths = instrument->getNumberParameter(COL_GAUGE_WIDTH_PARAM, true);
    if (!widths.empty() && widths.front() > 0.0) {
      collimator =
          std::make_unique<RadialCollimatorProfile>(widths.front(), instrument->getReferenceFrame()->vecPointingUp());
    }
  }
  g_log.information(collimator ? "Applying the calibrated radial collimator acceptance"
                               : "No calibrated collimator width on the instrument - collimator acceptance ignored");

  // The collimator only bounds the horizontal direction transverse to each detector's viewing axis. The
  // remaining directions have to be bounded by the incident beam profile or by an explicit gauge volume,
  // otherwise every element of the sample is weighted by attenuation alone along those directions and the
  // result is not a gauge volume centroid at all.
  if (collimator && !beamProfile->hasSpatialProfile() && !m_inputWS->run().hasProperty("GaugeVolume")) {
    g_log.warning("The collimator acceptance only restricts the direction transverse to each detector's "
                  "viewing axis, but the incident beam profile is uniform and no GaugeVolume sample log is "
                  "set, so nothing bounds the scattering volume along the viewing axis or vertically. The "
                  "result will be an attenuation-weighted average over the whole sample rather than a gauge "
                  "volume centre of mass - define a GaugeVolume, or set beam divergence with SetBeam.");
  }

  // Take a copy - points() returns by value, so a reference into it would dangle. The attenuation is
  // summed over these, and the cost of the per-detector loop is linear in their number, so sample a
  // bounded subset of a finely binned workspace. The centroid is insensitive to this: it depends on
  // how mu*L varies across a few-mm gauge, and mu itself varies only ~10% across a typical band.
  const auto lambdaPoints = m_inputWS->points(0);
  std::vector<double> lambdas;
  if (lambdaPoints.size() <= MAX_WAVELENGTH_SAMPLES) {
    lambdas.assign(lambdaPoints.begin(), lambdaPoints.end());
  } else {
    lambdas.reserve(MAX_WAVELENGTH_SAMPLES);
    const double step = static_cast<double>(lambdaPoints.size() - 1) / static_cast<double>(MAX_WAVELENGTH_SAMPLES - 1);
    for (size_t s = 0; s < MAX_WAVELENGTH_SAMPLES; ++s) {
      lambdas.emplace_back(lambdaPoints[static_cast<size_t>(std::round(static_cast<double>(s) * step))]);
    }
    g_log.information("Sampling " + std::to_string(MAX_WAVELENGTH_SAMPLES) + " of the workspace's " +
                      std::to_string(lambdaPoints.size()) + " wavelength points for the attenuation weighting");
  }

  const V3D beamDirection = instrument->getBeamDirection();
  // The collimator's focal point is fixed in the laboratory - the sample is translated through it. On
  // ENGIN-X the sample component sits at the instrument origin, which is that focal point, so it is
  // taken from there. It must not be replaced with an actual sample centre: that would drag the
  // collimator's field of view along with the sample.
  const V3D collimatorFocalPoint = instrument->getSample()->getPos();

  // Detector independent per element: the incident intensity and the incoming attenuation path.
  std::vector<V3D> livePoints;
  std::vector<double> incidentWeights;
  livePoints.reserve(elements.size());
  incidentWeights.reserve(elements.size());
  for (const auto &element : elements) {
    const double incidentIntensity = beamProfile->intensityAt(element);
    if (incidentIntensity <= 0.0) {
      continue; // not illuminated
    }
    livePoints.emplace_back(element);
    incidentWeights.emplace_back(incidentIntensity);
  }
  if (livePoints.empty()) {
    const std::string mess("The incident beam does not illuminate any of the scattering volume - check the beam "
                           "definition set by SetBeam against the gauge volume position");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }

  g_log.notice("Calculating scattering centres for " + std::to_string(livePoints.size()) + " volume elements over " +
               std::to_string(nHist) + " spectra");

  // Attenuation along the incoming leg is shared by every detector, so trace it once per element.
  std::vector<std::vector<double>> incidentAttenuation(livePoints.size(), std::vector<double>(lambdas.size(), 0.0));
  const V3D toSourceInShapeFrame = gonioIsIdentity ? -beamDirection : gonioRInv * (-beamDirection);
  for (size_t e = 0; e < livePoints.size(); ++e) {
    Geometry::Track incoming(toShapeFrame(livePoints[e]), toSourceInShapeFrame);
    sampleObject.interceptSurface(incoming);
    for (size_t l = 0; l < lambdas.size(); ++l) {
      incidentAttenuation[e][l] = incoming.calculateAttenuation(lambdas[l]);
    }
  }

  centres.assign(nHist, V3D(0.0, 0.0, 0.0));
  weights.assign(nHist, 0.0);

  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS))
  for (int64_t idx = 0; idx < static_cast<int64_t>(nHist); ++idx) {
    PARALLEL_START_INTERRUPT_REGION
    const auto i = static_cast<size_t>(idx);
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i)) {
      continue;
    }
    const auto detPos = spectrumInfo.position(i);

    V3D weightedSum(0.0, 0.0, 0.0);
    double summedWeight = 0.0;
    for (size_t e = 0; e < livePoints.size(); ++e) {
      const auto &element = livePoints[e];
      const double acceptance = collimator ? collimator->intensityAt(element, collimatorFocalPoint, detPos) : 1.0;
      if (acceptance <= 0.0) {
        continue;
      }
      const auto elementInShapeFrame = toShapeFrame(element);
      auto toDetector = toShapeFrame(detPos) - elementInShapeFrame;
      if (toDetector.norm2() == 0.0) {
        continue;
      }
      toDetector.normalize();
      Geometry::Track outgoing(elementInShapeFrame, toDetector);
      sampleObject.interceptSurface(outgoing);

      double attenuation = 0.0;
      for (size_t l = 0; l < lambdas.size(); ++l) {
        attenuation += incidentAttenuation[e][l] * outgoing.calculateAttenuation(lambdas[l]);
      }
      const double weight = incidentWeights[e] * acceptance * attenuation;
      weightedSum += element * weight;
      summedWeight += weight;
    }
    if (summedWeight > 0.0) {
      centres[i] = weightedSum / summedWeight;
      weights[i] = summedWeight;
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

/// Build the integration elements making up the scattering volume, as voxel centres in the lab frame.
///
/// The sample shape on the workspace already has any initial rotation baked into its definition,
/// so it is expressed in the sample shape's own frame. The workspace's goniometer R describes
/// the additional rotation from that frame into the lab frame. The gauge volume (if any) is
/// defined in the lab frame.
///
/// When a gauge volume is present we rasterise it in the lab frame and transform each candidate
/// voxel into the sample shape's frame via R.inv() to test inclusion against the sample. Doing
/// the intersection this way - rather than rotating the gauge into the sample frame - keeps the
/// gauge's axis-aligned bounding box tight even for non-axis-aligned rotations; rotating the
/// gauge would inflate its bbox and silently admit voxels outside the actual gauge volume.
///
/// With no gauge volume the illumination volume equals the sample, so we rasterise the sample
/// in its own frame (where the rasterise loop only ever accepts points inside the sample anyway)
/// and rotate each accepted element into the lab frame.
const std::vector<V3D> EstimateScatteringVolumeCentreOfMass::generateScatteringVolumeElements(
    const Geometry::IObject &sampleObject, const Kernel::Matrix<double> &gonioR, const V3D &beamDirection) {
  std::vector<V3D> elements;
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    elements = rasterizeLabGaugeElements(sampleObject, gonioR);
  } else {
    const auto raster = Geometry::Rasterize::calculate(beamDirection, sampleObject, sampleObject, m_cubeSide);
    const bool gonioIsIdentity = (gonioR == Kernel::Matrix<double>(3, 3, true));
    elements.reserve(raster.position.size());
    for (const auto &posInShapeFrame : raster.position) {
      elements.emplace_back(gonioIsIdentity ? posInShapeFrame : gonioR * posInShapeFrame);
    }
  }
  if (elements.empty()) {
    // most errors should be caught by the rasterise function, but just in case
    const std::string mess("Failed to find any points in the rasterized illumination volume within the sample shape - "
                           "Check sample shape and gauge volume are defined correctly or try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
  return elements;
}

/// Create the sample object using the Geometry classes, or use the existing one.
/// The shape is validated in validateInputs.
const Geometry::IObject_sptr EstimateScatteringVolumeCentreOfMass::extractValidSampleObject(const API::Sample &sample) {
  g_log.information("Successfully extracted the sample object");
  return sample.getShapePtr();
}

const std::vector<V3D>
EstimateScatteringVolumeCentreOfMass::rasterizeLabGaugeElements(const Geometry::IObject &sampleObject,
                                                                const Kernel::Matrix<double> &gonioR) {
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
  // validateInputs guarantees the gauge spans at least one element in every dimension
  const auto numXSlices = static_cast<size_t>(xLength / m_cubeSide);
  const auto numYSlices = static_cast<size_t>(yLength / m_cubeSide);
  const auto numZSlices = static_cast<size_t>(zLength / m_cubeSide);
  const double dx = xLength / static_cast<double>(numXSlices);
  const double dy = yLength / static_cast<double>(numYSlices);
  const double dz = zLength / static_cast<double>(numZSlices);

  std::vector<V3D> elements;
  elements.reserve(numXSlices * numYSlices * numZSlices);
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
        elements.emplace_back(pLab);
      }
    }
  }
  if (elements.empty()) {
    const std::string mess("Failed to find any voxels inside both the gauge volume and the sample "
                           "shape - check sample shape and gauge volume are defined correctly or "
                           "try reducing the ElementSize");
    g_log.error(mess);
    throw std::runtime_error(mess);
  }
  return elements;
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
