// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTypes/SpectrumDefinition.h"

using Mantid::Geometry::rad2deg;

namespace {
/**
 * @brief Calculate the min and max 2theta of a detector.
 * The 2theta are calculated only for the centre point and the 'top'
 * point, assuming that the width is zero. This works adequately well
 * for high aspect ratio detectors.
 * @param detInfo a DetectorInfo object
 * @param detIndex index of the detector within detInfo
 * @param samplePos a V3D pointing to the sample position
 * @param beamDir a unit vector pointing along the beam axis
 * @param upDir a unit vector pointing up
 * @return a pair (min(2theta), max(2theta)
 */
std::pair<double, double>
minMaxTheta(const Mantid::Geometry::DetectorInfo &detInfo,
            const size_t detIndex, const Mantid::Kernel::V3D &samplePos,
            const Mantid::Kernel::V3D &beamDir,
            const Mantid::Kernel::V3D &upDir) {
  const auto shape = detInfo.detector(detIndex).shape();
  const Mantid::Geometry::BoundingBox bbox = shape->getBoundingBox();
  const auto maxPoint = bbox.maxPoint() - samplePos;
  auto top = maxPoint * upDir;
  const auto rotation = detInfo.rotation(detIndex);
  const auto position = detInfo.position(detIndex);
  rotation.rotate(top);
  top += position;
  const auto topAngle = top.angle(beamDir);
  const auto centre = position - samplePos;
  const auto centreAngle = centre.angle(beamDir);
  return std::pair<double, double>(std::min(topAngle, centreAngle),
                                   std::max(topAngle, centreAngle));
}
} // namespace

namespace Mantid {
namespace Algorithms {
// Setup typedef for later use
using SpectraDistanceMap = std::map<specnum_t, Mantid::Kernel::V3D>;
using DetConstPtr = Geometry::IDetector_const_sptr;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQWNormalisedPolygon)

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

/**
 * @return the name of the Algorithm
 */
const std::string SofQWNormalisedPolygon::name() const {
  return "SofQWNormalisedPolygon";
}

/**
 * @return the version number of the Algorithm
 */
int SofQWNormalisedPolygon::version() const { return 1; }

/**
 * @return the category list for the Algorithm
 */
const std::string SofQWNormalisedPolygon::category() const {
  return "Inelastic\\SofQW";
}

/**
 * Initialize the algorithm
 */
void SofQWNormalisedPolygon::init() {
  SofQW::createCommonInputProperties(*this);
}

/**
 * Execute the algorithm.
 */
void SofQWNormalisedPolygon::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(*inputWS)) {
    throw std::invalid_argument(
        "The input workspace must have common binning across all spectra");
  }

  // Compute input caches
  m_EmodeProperties.initCachedValues(*inputWS, this);

  RebinnedOutput_sptr outputWS = SofQW::setUpOutputWorkspace<RebinnedOutput>(
      *inputWS, getProperty("QAxisBinning"), m_Qout,
      getProperty("EAxisBinning"), m_EmodeProperties);
  g_log.debug() << "Workspace type: " << outputWS->id() << '\n';
  setProperty("OutputWorkspace", outputWS);
  const size_t nEnergyBins = inputWS->blocksize();
  const size_t nHistos = inputWS->getNumberHistograms();

  // Holds the spectrum-detector mapping
  std::vector<SpectrumDefinition> detIDMapping(outputWS->getNumberHistograms());

  // Progress reports & cancellation
  const size_t nreports(nHistos * nEnergyBins);
  m_progress = boost::make_shared<API::Progress>(this, 0.0, 1.0, nreports);

  std::vector<double> par =
      inputWS->getInstrument()->getNumberParameter("detector-neighbour-offset");
  if (par.empty()) {
    // Index theta cache
    this->initAngularCachesNonPSD(*inputWS);
  } else {
    g_log.debug() << "Offset: " << par[0] << '\n';
    this->m_detNeighbourOffset = static_cast<int>(par[0]);
    this->initAngularCachesPSD(*inputWS);
  }
  const auto &X = inputWS->x(0);

  const auto &inputIndices = inputWS->indexInfo();
  const auto &spectrumInfo = inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(nHistos);
       ++i) // signed for openmp
  {
    PARALLEL_START_INTERUPT_REGION

    if (spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto *det =
        m_EmodeProperties.m_emode == 1 ? nullptr : &spectrumInfo.detector(i);

    const double theta = this->m_theta[i];
    const double thetaWidth = this->m_thetaWidths[i];

    // Compute polygon points
    const double thetaHalfWidth = 0.5 * thetaWidth;

    const double thetaLower = theta - thetaHalfWidth;
    const double thetaUpper = theta + thetaHalfWidth;

    const auto specNo = static_cast<specnum_t>(inputIndices.spectrumNumber(i));
    std::stringstream logStream;
    for (size_t j = 0; j < nEnergyBins; ++j) {
      m_progress->report("Computing polygon intersections");
      // For each input polygon test where it intersects with
      // the output grid and assign the appropriate weights of Y/E
      const double dE_j = X[j];
      const double dE_jp1 = X[j + 1];

      const double lrQ = m_EmodeProperties.q(dE_jp1, thetaLower, det);

      const V2D ll(dE_j, m_EmodeProperties.q(dE_j, thetaLower, det));
      const V2D lr(dE_jp1, lrQ);
      const V2D ur(dE_jp1, m_EmodeProperties.q(dE_jp1, thetaUpper, det));
      const V2D ul(dE_j, m_EmodeProperties.q(dE_j, thetaUpper, det));
      if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
        logStream << "Spectrum=" << specNo << ", theta=" << theta
                  << ",thetaWidth=" << thetaWidth << ". QE polygon: ll=" << ll
                  << ", lr=" << lr << ", ur=" << ur << ", ul=" << ul << "\n";
      }

      Quadrilateral inputQ = Quadrilateral(ll, lr, ur, ul);

      FractionalRebinning::rebinToFractionalOutput(inputQ, inputWS, i, j,
                                                   *outputWS, m_Qout);

      // Find which q bin this point lies in
      const MantidVec::difference_type qIndex =
          std::upper_bound(m_Qout.begin(), m_Qout.end(), lrQ) - m_Qout.begin();
      if (qIndex != 0 && qIndex < static_cast<int>(m_Qout.size())) {
        // Add this spectra-detector pair to the mapping
        PARALLEL_CRITICAL(SofQWNormalisedPolygon_spectramap) {
          // Could do a more complete merge of spectrum definitions here, but
          // historically only the ID of the first detector in the spectrum is
          // used, so I am keeping that for now.
          detIDMapping[qIndex - 1].add(
              spectrumInfo.spectrumDefinition(i)[0].first);
        }
      }
    }
    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      g_log.debug(logStream.str());
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->finalize();
  FractionalRebinning::normaliseOutput(outputWS, inputWS, m_progress);

  // Set the output spectrum-detector mapping
  auto outputIndices = outputWS->indexInfo();
  outputIndices.setSpectrumDefinitions(std::move(detIDMapping));
  outputWS->setIndexInfo(outputIndices);

  // Replace any NaNs in outputWorkspace with zeroes
  if (this->getProperty("ReplaceNaNs")) {
    auto replaceNans = this->createChildAlgorithm("ReplaceSpecialValues");
    replaceNans->setChild(true);
    replaceNans->initialize();
    replaceNans->setProperty("InputWorkspace", outputWS);
    replaceNans->setProperty("OutputWorkspace", outputWS);
    replaceNans->setProperty("NaNValue", 0.0);
    replaceNans->setProperty("InfinityValue", 0.0);
    replaceNans->setProperty("BigNumberThreshold", DBL_MAX);
    replaceNans->execute();
  }
}

/**
 * A map detector ID and Q ranges
 * This method looks unnecessary as it could be calculated on the fly but
 * the parallelization means that lazy instantation slows it down due to the
 * necessary CRITICAL sections required to update the cache. The Q range
 * values are required very frequently so the total time is more than
 * offset by this precaching step
 */
void SofQWNormalisedPolygon::initAngularCachesNonPSD(
    const MatrixWorkspace &workspace) {
  const size_t nhist = workspace.getNumberHistograms();
  constexpr double skipDetector{-1.};
  this->m_theta = std::vector<double>(nhist, skipDetector);
  this->m_thetaWidths = std::vector<double>(nhist, skipDetector);

  auto inst = workspace.getInstrument();
  const auto referenceFrame = inst->getReferenceFrame();
  const auto beamDir = referenceFrame->vecPointingAlongBeam();
  const auto upDir = referenceFrame->vecPointingUp();

  const auto &detectorInfo = workspace.detectorInfo();
  const auto &spectrumInfo = workspace.spectrumInfo();
  const auto samplePos = spectrumInfo.samplePosition();
  for (size_t i = 0; i < nhist; ++i) {
    m_progress->report("Calculating detector angles");

    // If no detector found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }

    const auto &det = spectrumInfo.detector(i);
    // Check to see if there is an EFixed, if not skip it
    try {
      m_EmodeProperties.getEFixed(det);
    } catch (std::runtime_error &) {
      continue;
    }

    this->m_theta[i] = spectrumInfo.twoTheta(i);

    if (spectrumInfo.hasUniqueDetector(i)) {
      const auto thetas =
          minMaxTheta(detectorInfo, i, samplePos, beamDir, upDir);
      m_thetaWidths[i] = 2. * (thetas.second - thetas.first);
    } else {
      const auto &group = dynamic_cast<const DetectorGroup &>(det);
      const auto ids = group.getDetectorIDs();
      double minTheta{std::numeric_limits<double>::max()};
      double maxTheta{std::numeric_limits<double>::lowest()};
      for (const auto id : ids) {
        const auto thetas = minMaxTheta(detectorInfo, detectorInfo.indexOf(id),
                                        samplePos, beamDir, upDir);
        minTheta = std::min(minTheta, thetas.first);
        maxTheta = std::max(maxTheta, thetas.second);
      }
      m_thetaWidths[i] = 2. * (maxTheta - minTheta);
      if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
        g_log.debug() << "Detector at spectrum = "
                      << workspace.getSpectrum(i).getSpectrumNo()
                      << ", width = " << m_thetaWidths[i] * rad2deg
                      << " degrees\n";
      }
    }
  }
}

/**
 * Function that retrieves the two-theta angle from a given
 * detector. It then looks up the nearest neighbours. Using those detectors,
 * it calculates the two-theta angular widths.
 * @param workspace : the workspace containing the needed detector information
 */
void SofQWNormalisedPolygon::initAngularCachesPSD(
    const MatrixWorkspace &workspace) {
  const size_t nHistos = workspace.getNumberHistograms();

  bool ignoreMasked = true;
  const int numNeighbours = 4;
  WorkspaceNearestNeighbourInfo neighbourInfo(workspace, ignoreMasked,
                                              numNeighbours);

  this->m_theta.resize(nHistos);
  this->m_thetaWidths.resize(nHistos);

  const auto &spectrumInfo = workspace.spectrumInfo();

  for (size_t i = 0; i < nHistos; ++i) {
    m_progress->report("Calculating detector angular widths");
    const specnum_t inSpec = workspace.getSpectrum(i).getSpectrumNo();
    const SpectraDistanceMap neighbours =
        neighbourInfo.getNeighboursExact(inSpec);

    // Convert from spectrum numbers to workspace indices
    double thetaWidth = std::numeric_limits<double>::lowest();

    // Find theta and phi widths
    const double theta = spectrumInfo.twoTheta(i);

    const specnum_t deltaPlus1 = inSpec + 1;
    const specnum_t deltaMinus1 = inSpec - 1;
    const specnum_t deltaPlusT = inSpec + this->m_detNeighbourOffset;
    const specnum_t deltaMinusT = inSpec - this->m_detNeighbourOffset;

    for (auto &neighbour : neighbours) {
      specnum_t spec = neighbour.first;
      if (spec == deltaPlus1 || spec == deltaMinus1 || spec == deltaPlusT ||
          spec == deltaMinusT) {
        const double theta_n = spectrumInfo.twoTheta(spec - 1) * 0.5;

        const double dTheta = std::abs(theta - theta_n);
        thetaWidth = std::max(thetaWidth, dTheta);
      }
    }
    this->m_theta[i] = theta;
    this->m_thetaWidths[i] = thetaWidth;
    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Detector at spectrum = " << inSpec
                    << ", width = " << thetaWidth * rad2deg << " degrees\n";
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
