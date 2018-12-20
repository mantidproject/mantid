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
#include "MantidGeometry/Objects/IObject.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/math/special_functions/pow.hpp>

using Mantid::Geometry::rad2deg;
using boost::math::pow;

namespace {
namespace Prop {
const std::string ANGULAR_WIDTHS{"AngularWidths"};
}
namespace Col {
const std::string DET_ID{"Detector ID"};
const std::string LOWER_2THETA{"Lower two theta"};
const std::string UPPER_2THETA{"Upper two theta"};
} // namespace Col

double twoThetaFromLocalPoint(const Mantid::Geometry::DetectorInfo &detInfo,
                              const size_t detIndex,
                              const Mantid::Kernel::V3D &samplePos,
                              const Mantid::Kernel::V3D &beamDir,
                              Mantid::Kernel::V3D point) {
  const auto rotation = detInfo.rotation(detIndex);
  const auto position = detInfo.position(detIndex);
  rotation.rotate(point);
  point += position;
  point -= samplePos;
  const auto twoTheta = point.angle(beamDir);
  return twoTheta;
}

std::pair<double, double>
cuboidTwoThetaRange(const Mantid::Geometry::DetectorInfo &detInfo,
                    const size_t detIndex, const Mantid::Kernel::V3D &samplePos,
                    const Mantid::Kernel::V3D &beamDir,
                    const std::vector<Mantid::Kernel::V3D> &shapeVectors) {
  // Convention from ShapeFactory::createGeometryHandler()
  const auto &leftFrontBottom = shapeVectors[0];
  const auto &leftFrontTop = shapeVectors[1];
  const auto &leftBackBottom = shapeVectors[2];
  const auto &rightFrontBottom = shapeVectors[3];
  const auto back = leftBackBottom - leftFrontBottom;
  const auto up = leftFrontTop - leftFrontBottom;
  const auto right = rightFrontBottom - leftFrontBottom;
  const std::array<Mantid::Kernel::V3D, 8> ring{
      leftFrontBottom,     leftFrontBottom + back * 0.5,
      leftBackBottom,      leftBackBottom + up * 0.5,
      leftBackBottom + up, leftFrontTop + back * 0.5,
      leftFrontTop,        leftFrontBottom + up * 0.5};
  double minTwoTheta{std::numeric_limits<double>::max()};
  double maxTwoTheta{std::numeric_limits<double>::lowest()};
  for (int width = 0; width < 3; ++width) {
    const auto offset = right * (0.5 * static_cast<double>(width));
    for (const auto &pointInRing : ring) {
      const auto point = pointInRing + offset;
      const auto current = twoThetaFromLocalPoint(detInfo, detIndex, samplePos,
                                                  beamDir, std::move(point));
      minTwoTheta = std::min(minTwoTheta, current);
      maxTwoTheta = std::max(maxTwoTheta, current);
    }
  }
  return std::make_pair(minTwoTheta, maxTwoTheta);
}

std::pair<double, double> cylinderTwoThetaRange(
    const Mantid::Geometry::DetectorInfo &detInfo, const size_t detIndex,
    const Mantid::Kernel::V3D &samplePos, const Mantid::Kernel::V3D &beamDir,
    const std::vector<Mantid::Kernel::V3D> &shapeVectors, const double radius,
    const double height) {
  // Convention from ShapeFactory::createGeometryHandler()
  const auto &centerOfBottom = shapeVectors[0];
  const auto &longAxisDir = shapeVectors[1];
  const auto inverseXYSumSq =
      1. / (pow<2>(longAxisDir.X()) + pow<2>(longAxisDir.Y()));
  const auto basisX = std::sqrt(1. - pow<2>(longAxisDir.X()) * inverseXYSumSq);
  const auto basisY = longAxisDir.X() * std::sqrt(inverseXYSumSq);
  const Mantid::Kernel::V3D basis1{basisX, basisY, 0.};
  const Mantid::Kernel::V3D basis2 = longAxisDir.cross_prod(basis1);
  const std::array<double, 8> angles{0.,          0.25 * M_PI, 0.5 * M_PI,
                                     0.75 * M_PI, M_PI,        1.25 * M_PI,
                                     1.5 * M_PI,  1.75 * M_PI};
  double minTwoTheta{std::numeric_limits<double>::max()};
  double maxTwoTheta{std::numeric_limits<double>::lowest()};
  for (size_t i = 0; i < angles.size(); ++i) {
    const auto basePoint =
        centerOfBottom +
        (basis1 * std::cos(angles[i]) + basis2 * std::sin(angles[i])) * radius;
    for (int i = 0; i < 3; ++i) {
      const auto point =
          basePoint + longAxisDir * (0.5 * height * static_cast<double>(i));
      const auto current = twoThetaFromLocalPoint(detInfo, detIndex, samplePos,
                                                  beamDir, std::move(point));
      minTwoTheta = std::min(minTwoTheta, current);
      maxTwoTheta = std::max(maxTwoTheta, current);
    }
  }
  return std::make_pair(minTwoTheta, maxTwoTheta);
}

/**
 * Calculate the 2theta at detector surface for a given direction.
 * @param detInfo a DetectorInfo object
 * @param detIndex index of the detector within detInfo
 * @param samplePos a V3D pointing to the sample position
 * @param beamDir a unit vector pointing along the beam axis
 * @param sideDir a unit vector pointing to the chosen direction
 * @return the 2theta in radians
 */
double twoThetaFromBoundingBox(const Mantid::Geometry::DetectorInfo &detInfo,
                               const size_t detIndex,
                               const Mantid::Kernel::V3D &samplePos,
                               const Mantid::Kernel::V3D &beamDir,
                               const Mantid::Kernel::V3D &sideDir) {
  const auto shape = detInfo.detector(detIndex).shape();
  const Mantid::Geometry::BoundingBox bbox = shape->getBoundingBox();
  const auto maxPoint = bbox.maxPoint();
  auto side = maxPoint * sideDir;
  return twoThetaFromLocalPoint(detInfo, detIndex, samplePos, beamDir, side);
}

std::pair<double, double> generalTwoThetaRange(
    const Mantid::Geometry::DetectorInfo &detInfo, const size_t detIndex,
    const Mantid::Kernel::V3D &samplePos, const Mantid::Kernel::V3D &beamDir) {
  double minTwoTheta{std::numeric_limits<double>::max()};
  double maxTwoTheta{std::numeric_limits<double>::lowest()};
  const std::array<Mantid::Kernel::V3D, 6> dirs{Mantid::Kernel::V3D{1., 0., 0.},
                                                {0., 1., 0.},
                                                {0., 0., 1.},
                                                {
                                                    -1.,
                                                    0.,
                                                    0.,
                                                },
                                                {0., -1., 0.},
                                                {0., 0., -1.}};
  for (const auto &dir : dirs) {
    const auto current =
        twoThetaFromBoundingBox(detInfo, detIndex, samplePos, beamDir, dir);
    minTwoTheta = std::min(minTwoTheta, current);
    maxTwoTheta = std::max(maxTwoTheta, current);
  }
  return std::make_pair(minTwoTheta, maxTwoTheta);
}

std::pair<double, double>
minMaxTwoTheta(const Mantid::Geometry::DetectorInfo &detInfo,
               const size_t detIndex, const Mantid::Kernel::V3D &samplePos,
               const Mantid::Kernel::V3D &beamDir) {
  const auto shape = detInfo.detector(detIndex).shape();
  Mantid::Geometry::detail::ShapeInfo::GeometryShape geometry;
  std::vector<Mantid::Kernel::V3D> shapeVectors;
  double radius;
  double height;
  shape->GetObjectGeom(geometry, shapeVectors, radius, height);
  switch (geometry) {
  case Mantid::Geometry::detail::ShapeInfo::GeometryShape::CUBOID:
    return cuboidTwoThetaRange(detInfo, detIndex, samplePos, beamDir,
                               shapeVectors);
  case Mantid::Geometry::detail::ShapeInfo::GeometryShape::CYLINDER:
    return cylinderTwoThetaRange(detInfo, detIndex, samplePos, beamDir,
                                 shapeVectors, radius, height);
  default:
    return generalTwoThetaRange(detInfo, detIndex, samplePos, beamDir);
  }
}

std::pair<double, double> twoThetasFromTable(
    const Mantid::detid_t detID, const std::vector<int> &detectorIDs,
    const std::vector<double> &lowers, const std::vector<double> &uppers) {
  const auto range = std::equal_range(detectorIDs.cbegin(), detectorIDs.cend(),
                                      static_cast<int>(detID));
  if (std::distance(range.first, range.second) > 1) {
    throw std::invalid_argument("Duplicate detector IDs in " +
                                Prop::ANGULAR_WIDTHS + " table.");
  }
  if (range.first == detectorIDs.cend()) {
    throw std::invalid_argument("No 2theta width found for detector ID " +
                                std::to_string(detID));
  }
  const auto index = std::distance(detectorIDs.cbegin(), range.first);
  return std::make_pair(lowers[index], uppers[index]);
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
  declareProperty(
      Kernel::make_unique<WorkspaceProperty<TableWorkspace>>(
          Prop::ANGULAR_WIDTHS, "", Direction::Input, PropertyMode::Optional),
      "A table workspace with a '" + Col::DET_ID +
          "' column listing detector IDs as well as '" + Col::LOWER_2THETA +
          "' and '" + Col::UPPER_2THETA +
          "' columns listing corresponding min and max 2thetas in "
          "radians.");
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

  // Index theta cache
  TableWorkspace_sptr widthTable = getProperty(Prop::ANGULAR_WIDTHS);
  if (widthTable) {
    initAngularCachesTable(*inputWS, *widthTable);
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
  for (int64_t i = 0; i < static_cast<int64_t>(nHistos);
       ++i) // signed for openmp
  {
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
        logStream << "Spectrum=" << specNo << ", lower theta=" << thetaLower
                  << ", upper theta=" << thetaUpper << ". QE polygon: ll=" << ll
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
  // TODO remove this debug output table from production release.
  auto table = WorkspaceFactory::Instance().createTable("TableWorkspace");
  table->addColumn("double", "Two theta");
  table->getColumn(0)->setPlotType(1);
  table->addColumn("double", Col::LOWER_2THETA);
  table->getColumn(1)->setPlotType(2);
  table->addColumn("double", Col::UPPER_2THETA);
  table->getColumn(2)->setPlotType(2);
  table->setRowCount(m_twoThetaLowers.size());
  for (size_t i = 0; i < m_twoThetaLowers.size(); ++i) {
    table->Double(i, 0) = spectrumInfo.twoTheta(i);
    table->Double(i, 1) = m_twoThetaLowers[i] - spectrumInfo.twoTheta(i);
    table->Double(i, 2) = m_twoThetaUppers[i] - spectrumInfo.twoTheta(i);
  }
  AnalysisDataService::Instance().addOrReplace("out_width_table", table);
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
    const MatrixWorkspace &workspace, const TableWorkspace &widthTable) {
  constexpr double skipDetector{-1.};
  const size_t nhist = workspace.getNumberHistograms();
  m_twoThetaLowers = std::vector<double>(nhist, skipDetector);
  m_twoThetaUppers = std::vector<double>(nhist, skipDetector);
  const auto &detIDs = widthTable.getColVector<int>(Col::DET_ID);
  const auto &lowers = widthTable.getColVector<double>(Col::LOWER_2THETA);
  const auto &uppers = widthTable.getColVector<double>(Col::UPPER_2THETA);
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
