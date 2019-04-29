// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/math/special_functions/pow.hpp>

using Mantid::Geometry::rad2deg;
using boost::math::pow;

namespace {
/**
 * Calculate 2theta for a point in detector's local coordinates.
 * @param detInfo a detector info
 * @param detInfoIndex an index to the detector info
 * @param samplePos position of the sample
 * @param beamDir a unit vector pointing in the beam direction
 * @param point a point in detector's local coordinates
 * @return 2theta angle in global coordinates, in radians
 */
double twoThetaFromLocalPoint(const Mantid::Geometry::DetectorInfo &detInfo,
                              const size_t detInfoIndex,
                              const Mantid::Kernel::V3D &samplePos,
                              const Mantid::Kernel::V3D &beamDir,
                              Mantid::Kernel::V3D point) {
  const auto rotation = detInfo.rotation(detInfoIndex);
  const auto position = detInfo.position(detInfoIndex);
  rotation.rotate(point);
  point += position;
  point -= samplePos;
  const auto twoTheta = point.angle(beamDir);
  return twoTheta;
}

/** Calculate min and max 2theta for cuboid shape.
 * Calculates the scattering angles for each corner and edge center and
 * returns the extrema.
 * @param detInfo a detector info
 * @param detInfoIndex an index to the detector info
 * @param samplePos position of the sample
 * @param beamDir a unit vector pointing in the beam direction
 * @param geometry a geometry object describing the cuboid
 * @return a pair (min(2theta), max(2theta)), units radians
 */
std::pair<double, double> cuboidTwoThetaRange(
    const Mantid::Geometry::DetectorInfo &detInfo, const size_t detInfoIndex,
    const Mantid::Kernel::V3D &samplePos, const Mantid::Kernel::V3D &beamDir,
    const Mantid::Geometry::detail::ShapeInfo::CuboidGeometry &geometry) {
  const auto back = geometry.leftBackBottom - geometry.leftFrontBottom;
  const auto up = geometry.leftFrontTop - geometry.leftFrontBottom;
  const auto right = geometry.rightFrontBottom - geometry.leftFrontBottom;
  const std::array<Mantid::Kernel::V3D, 8> capRing{
      {geometry.leftFrontBottom, geometry.leftFrontBottom + back * 0.5,
       geometry.leftBackBottom, geometry.leftBackBottom + up * 0.5,
       geometry.leftBackBottom + up, geometry.leftFrontTop + back * 0.5,
       geometry.leftFrontTop, geometry.leftFrontBottom + up * 0.5}};
  double minTwoTheta{std::numeric_limits<double>::max()};
  double maxTwoTheta{std::numeric_limits<double>::lowest()};
  for (int width = 0; width < 2; ++width) {
    const auto offset = right * static_cast<double>(width);
    for (const auto &pointInRing : capRing) {
      auto point = pointInRing;
      if (width != 0) {
        point += offset;
      }
      const auto current = twoThetaFromLocalPoint(
          detInfo, detInfoIndex, samplePos, beamDir, std::move(point));
      minTwoTheta = std::min(minTwoTheta, current);
      maxTwoTheta = std::max(maxTwoTheta, current);
    }
  }
  const auto beltOffset = right * 0.5;
  for (size_t beltIndex = 0; beltIndex < capRing.size(); beltIndex += 2) {
    const auto point = capRing[beltIndex] + beltOffset;
    const auto current = twoThetaFromLocalPoint(
        detInfo, detInfoIndex, samplePos, beamDir, std::move(point));
    minTwoTheta = std::min(minTwoTheta, current);
    maxTwoTheta = std::max(maxTwoTheta, current);
  }
  return std::make_pair(minTwoTheta, maxTwoTheta);
}

/** Calculate min and max 2theta for cylinder shape.
 * Calculates the scattering angles at a number of points around the
 * outer rim at top, center and bottom of the cylinder.
 * @param detInfo a detector info
 * @param detInfoIndex an index to the detector info, not detector ID
 * @param samplePos position of the sample
 * @param beamDir a unit vector pointing in the beam direction
 * @param geometry a geometry object describing the cylinder
 * @return a pair (min(2theta), max(2theta)), units radians
 */

std::pair<double, double> cylinderTwoThetaRange(
    const Mantid::Geometry::DetectorInfo &detInfo, const size_t detInfoIndex,
    const Mantid::Kernel::V3D &samplePos, const Mantid::Kernel::V3D &beamDir,
    const Mantid::Geometry::detail::ShapeInfo::CylinderGeometry &geometry) {
  Mantid::Kernel::V3D basis1{1., 0., 0.};
  if (geometry.axis.X() != 0. && geometry.axis.Z() != 0) {
    const auto inverseXZSumSq =
        1. / (pow<2>(geometry.axis.X()) + pow<2>(geometry.axis.Z()));
    basis1.setX(std::sqrt(1. - pow<2>(geometry.axis.X()) * inverseXZSumSq));
    basis1.setY(geometry.axis.X() * std::sqrt(inverseXZSumSq));
  }
  const Mantid::Kernel::V3D basis2 = geometry.axis.cross_prod(basis1);
  const std::array<double, 8> angles{{0., 0.25 * M_PI, 0.5 * M_PI, 0.75 * M_PI,
                                      M_PI, 1.25 * M_PI, 1.5 * M_PI,
                                      1.75 * M_PI}};
  double minTwoTheta{std::numeric_limits<double>::max()};
  double maxTwoTheta{std::numeric_limits<double>::lowest()};
  for (const double &angle : angles) {
    const auto basePoint =
        geometry.centreOfBottomBase +
        (basis1 * std::cos(angle) + basis2 * std::sin(angle)) * geometry.radius;
    for (int i = 0; i < 3; ++i) {
      const auto point = basePoint + geometry.axis * (0.5 * geometry.height *
                                                      static_cast<double>(i));
      const auto current = twoThetaFromLocalPoint(
          detInfo, detInfoIndex, samplePos, beamDir, std::move(point));
      minTwoTheta = std::min(minTwoTheta, current);
      maxTwoTheta = std::max(maxTwoTheta, current);
    }
  }
  return std::make_pair(minTwoTheta, maxTwoTheta);
}

/**
 * Calculate the 2theta at bounding box surface for a given direction.
 * @param detInfo a DetectorInfo object
 * @param detInfoIndex index of the detector within detInfo
 * @param samplePos a V3D pointing to the sample position
 * @param beamDir a unit vector pointing along the beam axis
 * @param sideDir a unit vector pointing to the chosen direction
 * @return the 2theta in radians
 */
double twoThetaFromBoundingBox(const Mantid::Geometry::DetectorInfo &detInfo,
                               const size_t detInfoIndex,
                               const Mantid::Kernel::V3D &samplePos,
                               const Mantid::Kernel::V3D &beamDir,
                               const Mantid::Kernel::V3D &sideDir) {
  const auto shape = detInfo.detector(detInfoIndex).shape();
  const Mantid::Geometry::BoundingBox bbox = shape->getBoundingBox();
  const auto maxPoint = bbox.maxPoint();
  auto side = maxPoint * sideDir;
  return twoThetaFromLocalPoint(detInfo, detInfoIndex, samplePos, beamDir,
                                side);
}

/** Calculate min and max 2theta for a general shape.
 * The calculation is done using the bounding box. The 2thetas are
 * computed for the centers of the six faces of the box.
 * @param detInfo a detector info
 * @param detInfoIndex an index to the detector info
 * @param samplePos position of the sample
 * @param beamDir a unit vector pointing in the beam direction
 * @return a pair (min(2theta), max(2theta)), in radians.
 */
std::pair<double, double> generalTwoThetaRange(
    const Mantid::Geometry::DetectorInfo &detInfo, const size_t detInfoIndex,
    const Mantid::Kernel::V3D &samplePos, const Mantid::Kernel::V3D &beamDir) {
  double minTwoTheta{std::numeric_limits<double>::max()};
  double maxTwoTheta{std::numeric_limits<double>::lowest()};
  const std::array<Mantid::Kernel::V3D, 6> dirs{
      {Mantid::Kernel::V3D{1., 0., 0.},
       {0., 1., 0.},
       {0., 0., 1.},
       {-1., 0., 0.},
       {0., -1., 0.},
       {0., 0., -1.}}};
  for (const auto &dir : dirs) {
    const auto current =
        twoThetaFromBoundingBox(detInfo, detInfoIndex, samplePos, beamDir, dir);
    minTwoTheta = std::min(minTwoTheta, current);
    maxTwoTheta = std::max(maxTwoTheta, current);
  }
  return std::make_pair(minTwoTheta, maxTwoTheta);
}

/**
 * Calculate the scattering angle extrema for a detector.
 * The chosen method depends on the detector shape.
 * @param detInfo a detector info
 * @param detInfoIndex an index to the detector info
 * @param samplePos position of the sample
 * @param beamDir a unit vector pointing in the beam direction
 * @return a pair (min(2theta), max(2theta)), in radians
 */
std::pair<double, double>
minMaxTwoTheta(const Mantid::Geometry::DetectorInfo &detInfo,
               const size_t detInfoIndex, const Mantid::Kernel::V3D &samplePos,
               const Mantid::Kernel::V3D &beamDir) {
  const auto shape = detInfo.detector(detInfoIndex).shape();
  const Mantid::Geometry::CSGObject *csgShape;
  switch (shape->shape()) {
  case Mantid::Geometry::detail::ShapeInfo::GeometryShape::CYLINDER:
    csgShape = dynamic_cast<const Mantid::Geometry::CSGObject *>(shape.get());
    assert(csgShape);
    return cylinderTwoThetaRange(detInfo, detInfoIndex, samplePos, beamDir,
                                 csgShape->shapeInfo().cylinderGeometry());
  case Mantid::Geometry::detail::ShapeInfo::GeometryShape::CUBOID:
    csgShape = dynamic_cast<const Mantid::Geometry::CSGObject *>(shape.get());
    assert(csgShape);
    return cuboidTwoThetaRange(detInfo, detInfoIndex, samplePos, beamDir,
                               csgShape->shapeInfo().cuboidGeometry());
  default:
    return generalTwoThetaRange(detInfo, detInfoIndex, samplePos, beamDir);
  }
}

/** Return the tabulated scattering angle extrema for a detector.
 * @param detID detector id for which to find the 2thetas
 * @param detectorIDs list of tabulated detector ids
 * @param lowers list of min(2theta) corresponding to an id in detectorIDs
 * @param uppers list of max(2theta) corresponding to an id in detectorIDs
 * @return a pair (min(2theta), max(2theta)), in radians
 */
std::pair<double, double> twoThetasFromTable(
    const Mantid::detid_t detID, const std::vector<int> &detectorIDs,
    const std::vector<double> &lowers, const std::vector<double> &uppers) {
  const auto range = std::equal_range(detectorIDs.cbegin(), detectorIDs.cend(),
                                      static_cast<int>(detID));
  if (std::distance(range.first, range.second) > 1) {
    throw std::invalid_argument(
        "Duplicate detector IDs in 'DetectorTwoThetaRanges'.");
  }
  if (range.first == detectorIDs.cend()) {
    throw std::invalid_argument("No min/max 2thetas found for detector ID " +
                                std::to_string(detID));
  }
  const auto index = std::distance(detectorIDs.cbegin(), range.first);
  const auto minmax = std::make_pair(lowers[index], uppers[index]);
  if (minmax.first <= 0) {
    throw std::invalid_argument("Non-positive min 2theta for detector ID " +
                                std::to_string(detID));
  }
  if (minmax.first > M_PI) {
    throw std::invalid_argument("Min 2theta greater than pi for detector ID " +
                                std::to_string(detID));
  }
  if (minmax.second > M_PI) {
    throw std::invalid_argument("Max 2theta greater than pi for detector ID" +
                                std::to_string(detID));
  }
  if (minmax.first >= minmax.second) {
    throw std::invalid_argument("Min 2theta larger than max for detector ID " +
                                std::to_string(detID));
  }
  return minmax;
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
  m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, nreports);

  // Index theta cache
  TableWorkspace_sptr twoThetaTable = getProperty("DetectorTwoThetaRanges");
  if (twoThetaTable) {
    initAngularCachesTable(*inputWS, *twoThetaTable);
  } else {
    std::vector<double> par = inputWS->getInstrument()->getNumberParameter(
        "detector-neighbour-offset");
    if (par.empty()) {
      this->initAngularCachesNonPSD(*inputWS);
    } else {
      g_log.debug() << "Offset: " << par[0] << '\n';
      this->m_detNeighbourOffset = static_cast<int>(par[0]);
      this->initAngularCachesPSD(*inputWS);
    }
  }
  const auto &X = inputWS->x(0);

  const auto &inputIndices = inputWS->indexInfo();
  const auto &spectrumInfo = inputWS->spectrumInfo();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(nHistos); ++i) {
    PARALLEL_START_INTERUPT_REGION

    if (spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto *det =
        m_EmodeProperties.m_emode == 1 ? nullptr : &spectrumInfo.detector(i);

    const double thetaLower = m_twoThetaLowers[i];
    const double thetaUpper = m_twoThetaUppers[i];

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
        logStream << "Spectrum=" << specNo
                  << ", lower theta=" << thetaLower * rad2deg
                  << ", upper theta=" << thetaUpper * rad2deg
                  << ". QE polygon: ll=" << ll << ", lr=" << lr << ", ur=" << ur
                  << ", ul=" << ul << "\n";
      }

      using FractionalRebinning::rebinToFractionalOutput;
      rebinToFractionalOutput(Quadrilateral(ll, lr, ur, ul), inputWS, i, j,
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
  FractionalRebinning::normaliseOutput(outputWS, inputWS, m_progress.get());

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
  m_twoThetaLowers = std::vector<double>(nhist, skipDetector);
  m_twoThetaUppers = std::vector<double>(nhist, skipDetector);

  auto inst = workspace.getInstrument();
  const auto referenceFrame = inst->getReferenceFrame();
  const auto beamDir = referenceFrame->vecPointingAlongBeam();

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

    double minTwoTheta{spectrumInfo.twoTheta(i)};
    double maxTwoTheta{minTwoTheta};
    if (spectrumInfo.hasUniqueDetector(i)) {
      const auto detInfoIndex = detectorInfo.indexOf(det.getID());
      std::tie(minTwoTheta, maxTwoTheta) =
          minMaxTwoTheta(detectorInfo, detInfoIndex, samplePos, beamDir);
    } else {
      const auto &group = dynamic_cast<const DetectorGroup &>(det);
      const auto ids = group.getDetectorIDs();
      for (const auto id : ids) {
        const auto detInfoIndex = detectorInfo.indexOf(id);
        const auto current =
            minMaxTwoTheta(detectorInfo, detInfoIndex, samplePos, beamDir);
        minTwoTheta = std::min(minTwoTheta, current.first);
        maxTwoTheta = std::max(maxTwoTheta, current.second);
      }
    }
    m_twoThetaLowers[i] = minTwoTheta;
    m_twoThetaUppers[i] = maxTwoTheta;
    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Detector at spectrum = "
                    << workspace.getSpectrum(i).getSpectrumNo()
                    << ", lower 2theta = " << m_twoThetaLowers[i] * rad2deg
                    << ", upper 2theta = " << m_twoThetaUppers[i] * rad2deg
                    << " degrees\n";
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

  m_twoThetaLowers.resize(nHistos);
  m_twoThetaUppers.resize(nHistos);

  const auto &spectrumInfo = workspace.spectrumInfo();

  for (size_t i = 0; i < nHistos; ++i) {
    m_progress->report("Calculating detector angular widths");

    // If no detector found, skip onto the next spectrum
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }

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
    m_twoThetaLowers[i] = theta - thetaWidth / 2.;
    m_twoThetaUppers[i] = theta + thetaWidth / 2.;
    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Detector at spectrum = " << inSpec
                    << ", width = " << thetaWidth * rad2deg << " degrees\n";
    }
  }
}

void SofQWNormalisedPolygon::initAngularCachesTable(
    const MatrixWorkspace &workspace, const TableWorkspace &angleTable) {
  constexpr double skipDetector{-1.};
  const size_t nhist = workspace.getNumberHistograms();
  m_twoThetaLowers = std::vector<double>(nhist, skipDetector);
  m_twoThetaUppers = std::vector<double>(nhist, skipDetector);
  const auto &detIDs = angleTable.getColVector<int>("Detector ID");
  const auto &lowers = angleTable.getColVector<double>("Min two theta");
  const auto &uppers = angleTable.getColVector<double>("Max two theta");
  const auto &spectrumInfo = workspace.spectrumInfo();
  for (size_t i = 0; i < nhist; ++i) {
    m_progress->report("Reading detector angles");
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto &det = spectrumInfo.detector(i);
    try {
      m_EmodeProperties.getEFixed(det);
    } catch (std::runtime_error &) {
      continue;
    }

    if (spectrumInfo.hasUniqueDetector(i)) {
      const auto twoThetas =
          twoThetasFromTable(det.getID(), detIDs, lowers, uppers);
      m_twoThetaLowers[i] = twoThetas.first;
      m_twoThetaUppers[i] = twoThetas.second;
    } else {
      const auto &group = dynamic_cast<const DetectorGroup &>(det);
      const auto ids = group.getDetectorIDs();
      double minTwoTheta{spectrumInfo.twoTheta(i)};
      double maxTwoTheta{minTwoTheta};
      for (const auto id : ids) {
        const auto twoThetas = twoThetasFromTable(id, detIDs, lowers, uppers);
        minTwoTheta = std::min(minTwoTheta, twoThetas.first);
        maxTwoTheta = std::max(maxTwoTheta, twoThetas.second);
      }
      m_twoThetaLowers[i] = minTwoTheta;
      m_twoThetaUppers[i] = maxTwoTheta;
    }
    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Detector at spectrum = "
                    << workspace.getSpectrum(i).getSpectrumNo()
                    << ", lower 2theta = " << m_twoThetaLowers[i] * rad2deg
                    << ", upper 2theta = " << m_twoThetaUppers[i] * rad2deg
                    << " degrees\n";
    }
  }
}
} // namespace Algorithms
} // namespace Mantid
