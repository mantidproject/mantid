// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/WeightedGaugeVolumeAbsorption.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/BeamProfileFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Unit.h"
#include <algorithm>
#include <cmath>

namespace Mantid::Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;
using namespace Mantid::DataObjects;

DECLARE_ALGORITHM(WeightedGaugeVolumeAbsorption)

namespace {
const std::string UNIT_M = "m";
const std::string UNIT_CM = "cm";
const std::string UNIT_MM = "mm";
const std::unordered_map<std::string, double> unitToMeters{{UNIT_M, 1.0}, {UNIT_CM, 0.01}, {UNIT_MM, 0.001}};
/// Instrument parameter holding the calibrated collimator gauge width, in metres.
const std::string COL_GAUGE_WIDTH_PARAM = "col-gauge-width";
/// Upper bound on the wavelengths summed over when locating the centre of gravity. The centroid is
/// insensitive to this: it depends on how mu*L varies across a few-mm gauge, and mu itself varies
/// only ~10% across a typical band.
constexpr size_t MAX_CENTRE_WAVELENGTHS = 50;
} // namespace

void WeightedGaugeVolumeAbsorption::defineProperties() {
  AnyShapeAbsorption::defineProperties();

  declareProperty("ElementUnits", UNIT_MM,
                  std::make_shared<StringListValidator>(std::vector<std::string>{UNIT_M, UNIT_CM, UNIT_MM}),
                  "The units which ElementSize has been provided in");

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("IlluminatedVolumeFraction", "", Direction::Output, PropertyMode::Optional),
      "Single-valued per spectrum: the fraction of the sample volume that is both lit by "
      "the beam and visible to that detector. Multiply by OutputWorkspace to normalise "
      "the attenuation factor by the whole sample rather than by the volume seen.");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("ScatteringCentres", "", Direction::Output,
                                                                       PropertyMode::Optional),
                  "Neutron weighted centre of gravity of the volume each detector sees, in the lab "
                  "frame and in metres. Columns: detid, x, y, z, weight.");
}

void WeightedGaugeVolumeAbsorption::retrieveProperties() {
  AnyShapeAbsorption::retrieveProperties();
  // The base class read ElementSize as millimetres; reinterpret it in the requested units.
  const double elementSizeInUnits = getProperty("ElementSize");
  const std::string elementUnits = getProperty("ElementUnits");
  m_cubeSide = elementSizeInUnits * unitToMeters.at(elementUnits);
}

std::map<std::string, std::string> WeightedGaugeVolumeAbsorption::validateInputs() {
  auto issues = AbsorptionCorrection::validateInputs();

  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  if (!inputWS) {
    return issues;
  }
  // Weighting the elements by attenuation is meaningless without something to attenuate in, and the
  // cross sections supplied on the algorithm are an equally valid way of providing it.
  const bool haveMaterial = inputWS->sample().getMaterial().numberDensity() > 0.0;
  const bool haveCrossSections = !isEmpty(static_cast<double>(getProperty("SampleNumberDensity")));
  if (!haveMaterial && !haveCrossSections) {
    issues["InputWorkspace"] = "This algorithm weights by attenuation, so it needs a sample material - set one with "
                               "SetSampleMaterial, or give SampleNumberDensity and the cross sections directly";
  }
  return issues;
}

/// Rasterise as the base class does, then cache the parts of the weight that do not depend on the
/// detector: the incident beam profile, and the collimator if the instrument carries one.
void WeightedGaugeVolumeAbsorption::initialiseCachedDistances() {
  AnyShapeAbsorption::initialiseCachedDistances();

  const auto instrument = m_inputWS->getInstrument();
  m_samplePos = instrument->getSample()->getPos();

  // P_i. The beam profile is expressed in the lab frame, as are the element positions.
  m_incidentIntensity.assign(m_numVolumeElements, 1.0);
  try {
    const auto beamProfile = BeamProfileFactory::createBeamProfile(*instrument, m_inputWS->sample());
    for (size_t i = 0; i < m_numVolumeElements; ++i) {
      m_incidentIntensity[i] = beamProfile->intensityAt(m_elementPositions[i]);
    }
  } catch (const std::invalid_argument &) {
    // No beam defined on the instrument, so the illumination is uniform. This is the common case
    // for a gauge volume, which already describes the lit region.
    g_log.information("No beam geometry defined - treating the gauge volume as uniformly illuminated");
  }

  const double totalIncident = std::accumulate(m_incidentIntensity.cbegin(), m_incidentIntensity.cend(), 0.0);
  if (totalIncident <= 0.0) {
    throw std::runtime_error("The incident beam does not illuminate any of the scattering volume - check the beam "
                             "definition set by SetBeam against the gauge volume position");
  }

  // P_d. Only applied when the collimator has been calibrated onto the workspace.
  if (instrument->hasParameter(COL_GAUGE_WIDTH_PARAM)) {
    const auto widths = instrument->getNumberParameter(COL_GAUGE_WIDTH_PARAM, true);
    if (!widths.empty() && widths.front() > 0.0) {
      m_collimator =
          std::make_unique<RadialCollimatorProfile>(widths.front(), instrument->getReferenceFrame()->vecPointingUp());
    }
  }
  g_log.information(m_collimator ? "Applying the calibrated radial collimator acceptance"
                                 : "No calibrated collimator width on the instrument - collimator acceptance ignored");

  // Wavelengths for the centre of gravity. Sub-sampled for the same reason the bin loop is.
  const auto points = m_inputWS->points(0);
  if (points.size() <= MAX_CENTRE_WAVELENGTHS) {
    m_lambdas.assign(points.begin(), points.end());
  } else {
    m_lambdas.reserve(MAX_CENTRE_WAVELENGTHS);
    const double step = static_cast<double>(points.size() - 1) / static_cast<double>(MAX_CENTRE_WAVELENGTHS - 1);
    for (size_t s = 0; s < MAX_CENTRE_WAVELENGTHS; ++s) {
      m_lambdas.emplace_back(points[static_cast<size_t>(std::round(static_cast<double>(s) * step))]);
    }
  }

  const auto nHist = m_inputWS->getNumberHistograms();
  m_scatteringCentres.assign(nHist, V3D(0.0, 0.0, 0.0));
  m_centreWeights.assign(nHist, 0.0);
  m_illuminatedFraction.assign(nHist, 0.0);
}

void WeightedGaugeVolumeAbsorption::calculateElementWeights(const IDetector &detector,
                                                            std::vector<double> &weights) const {
  weights.resize(m_numVolumeElements);
  if (!m_collimator) {
    weights.assign(m_incidentIntensity.cbegin(), m_incidentIntensity.cend());
    return;
  }
  const auto detectorPos = detector.getPos();
  for (size_t i = 0; i < m_numVolumeElements; ++i) {
    weights[i] = m_incidentIntensity[i] * m_collimator->intensityAt(m_elementPositions[i], m_samplePos, detectorPos);
  }
}

/// Accumulate the neutron weighted centre of gravity for this detector from the same L1, L2 and
/// element weights the attenuation integral just used.
void WeightedGaugeVolumeAbsorption::perSpectrumHook(size_t wsIndex, const std::vector<double> &L2s,
                                                    const std::vector<double> &weights) {
  // Reading the material from the traced object picks up any cross sections supplied on the
  // algorithm, since the base class clones them onto the shape before the integration starts.
  const auto &material = m_sampleObject->material();

  V3D weightedSum(0.0, 0.0, 0.0);
  double summedWeight = 0.0;
  double illuminatedVolume = 0.0;
  for (size_t i = 0; i < m_numVolumeElements; ++i) {
    const double elementWeight = m_elementVolumes[i] * (weights.empty() ? 1.0 : weights[i]);
    if (elementWeight <= 0.0) {
      continue;
    }
    illuminatedVolume += elementWeight;

    const double pathLength = m_L1s[i] + L2s[i];
    double attenuation = 0.0;
    for (const auto lambda : m_lambdas) {
      attenuation += std::exp(-material.attenuationCoefficient(lambda) * pathLength);
    }

    const double weight = elementWeight * attenuation;
    weightedSum += m_elementPositions[i] * weight;
    summedWeight += weight;
  }

  if (summedWeight > 0.0) {
    m_scatteringCentres[wsIndex] = weightedSum / summedWeight;
    m_centreWeights[wsIndex] = summedWeight;
  }
  // Deliberately not m_sampleVolume: with a gauge volume the base class sets that to the rasterised
  // gauge-and-sample volume, so dividing by it would return one by construction. The whole sample
  // shape is what makes this a meaningful fraction.
  const double wholeSampleVolume = m_sampleObject->volume();
  m_illuminatedFraction[wsIndex] = wholeSampleVolume > 0.0 ? illuminatedVolume / wholeSampleVolume : 0.0;
}

void WeightedGaugeVolumeAbsorption::exec() {
  AbsorptionCorrection::exec();
  setDerivedOutputs();
}

void WeightedGaugeVolumeAbsorption::setDerivedOutputs() {
  if (!getPropertyValue("IlluminatedVolumeFraction").empty()) {
    MatrixWorkspace_sptr fraction =
        create<Workspace2D>(*m_inputWS, m_illuminatedFraction.size(), HistogramData::Points(1));
    fraction->setYUnitLabel("Illuminated volume fraction");
    for (size_t i = 0; i < m_illuminatedFraction.size(); ++i) {
      fraction->mutableX(i)[0] = 0.0;
      fraction->mutableY(i)[0] = m_illuminatedFraction[i];
      fraction->mutableE(i)[0] = 0.0;
    }
    setProperty("IlluminatedVolumeFraction", fraction);
  }

  if (getPropertyValue("ScatteringCentres").empty()) {
    return;
  }

  auto table = WorkspaceFactory::Instance().createTable();
  table->addColumn("int", "detid");
  table->addColumn("double", "x");
  table->addColumn("double", "y");
  table->addColumn("double", "z");
  table->addColumn("double", "weight");

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  size_t unseenCount = 0;
  for (size_t i = 0; i < m_centreWeights.size(); ++i) {
    if (m_centreWeights[i] <= 0.0) {
      if (spectrumInfo.hasDetectors(i) && !spectrumInfo.isMonitor(i) && !spectrumInfo.isMasked(i)) {
        ++unseenCount;
      }
      continue;
    }
    const auto detids = m_inputWS->getSpectrum(i).getDetectorIDs();
    // Share the spectrum's weight between its detectors rather than repeating it, so that summing
    // the column over the table recovers the total.
    const double weightPerDetector = m_centreWeights[i] / static_cast<double>(detids.size());
    for (const auto detid : detids) {
      TableRow row = table->appendRow();
      row << static_cast<int>(detid) << m_scatteringCentres[i].X() << m_scatteringCentres[i].Y()
          << m_scatteringCentres[i].Z() << weightPerDetector;
    }
  }
  if (unseenCount > 0) {
    g_log.warning("No part of the scattering volume is visible to " + std::to_string(unseenCount) +
                  " detectors - they have been left out of the scattering centre table");
  }
  if (table->rowCount() == 0) {
    throw std::runtime_error("No detector sees any part of the scattering volume - check the sample shape, gauge "
                             "volume, beam and collimator definitions");
  }
  setProperty("ScatteringCentres", table);
}

} // namespace Mantid::Algorithms
