#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidTypes/SpectrumDefinition.h"

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

/// Default constructor
SofQWNormalisedPolygon::SofQWNormalisedPolygon()
    : Rebin2D(), m_Qout(), m_thetaWidth(0.0), m_detNeighbourOffset(-1) {}

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
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // Do the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(*inputWS)) {
    throw std::invalid_argument(
        "The input workspace must have common binning across all spectra");
  }

  RebinnedOutput_sptr outputWS =
      this->setUpOutputWorkspace(*inputWS, getProperty("QAxisBinning"), m_Qout,
                                 getProperty("EAxisBinning"));
  g_log.debug() << "Workspace type: " << outputWS->id() << '\n';
  setProperty("OutputWorkspace", outputWS);
  const size_t nEnergyBins = inputWS->blocksize();
  const size_t nHistos = inputWS->getNumberHistograms();

  // Holds the spectrum-detector mapping
  std::vector<SpectrumDefinition> detIDMapping(outputWS->getNumberHistograms());

  // Progress reports & cancellation
  const size_t nreports(nHistos * nEnergyBins);
  m_progress = boost::shared_ptr<API::Progress>(
      new API::Progress(this, 0.0, 1.0, nreports));

  // Compute input caches
  m_EmodeProperties.initCachedValues(*inputWS, this);

  std::vector<double> par =
      inputWS->getInstrument()->getNumberParameter("detector-neighbour-offset");
  if (par.empty()) {
    // Index theta cache
    this->initAngularCachesNonPSD(inputWS);
  } else {
    g_log.debug() << "Offset: " << par[0] << '\n';
    this->m_detNeighbourOffset = static_cast<int>(par[0]);
    this->initAngularCachesPSD(inputWS);
  }

  const auto &X = inputWS->x(0);
  int emode = m_EmodeProperties.m_emode;

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

    double theta = this->m_theta[i];
    double phi = this->m_phi[i];
    double thetaWidth = this->m_thetaWidths[i];
    double phiWidth = this->m_phiWidths[i];

    // Compute polygon points
    double thetaHalfWidth = 0.5 * thetaWidth;
    double phiHalfWidth = 0.5 * phiWidth;

    const double thetaLower = theta - thetaHalfWidth;
    const double thetaUpper = theta + thetaHalfWidth;

    const double phiLower = phi - phiHalfWidth;
    const double phiUpper = phi + phiHalfWidth;

    const double efixed = m_EmodeProperties.getEFixed(spectrumInfo.detector(i));
    const auto specNo = static_cast<specnum_t>(inputIndices.spectrumNumber(i));
    std::stringstream logStream;
    for (size_t j = 0; j < nEnergyBins; ++j) {
      m_progress->report("Computing polygon intersections");
      // For each input polygon test where it intersects with
      // the output grid and assign the appropriate weights of Y/E
      const double dE_j = X[j];
      const double dE_jp1 = X[j + 1];

      const double lrQ =
          this->calculateQ(efixed, emode, dE_jp1, thetaLower, phiLower);

      const V2D ll(dE_j,
                   this->calculateQ(efixed, emode, dE_j, thetaLower, phiLower));
      const V2D lr(dE_jp1, lrQ);
      const V2D ur(dE_jp1, this->calculateQ(efixed, emode, dE_jp1, thetaUpper,
                                            phiUpper));
      const V2D ul(dE_j,
                   this->calculateQ(efixed, emode, dE_j, thetaUpper, phiUpper));
      if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
        logStream << "Spectrum=" << specNo << ", theta=" << theta
                  << ",thetaWidth=" << thetaWidth << ", phi=" << phi
                  << ", phiWidth=" << phiWidth << ". QE polygon: ll=" << ll
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
 * Calculate the Q value for a given set of energy transfer, scattering
 * and azimuthal angle.
 * @param efixed :: An fixed energy value
 * @param emode  :: the energy evaluation mode
 * @param deltaE :: The energy change
 * @param twoTheta :: The value of the scattering angle
 * @param azimuthal :: The value of the azimuthual angle
 * @return The value of Q
 */
double SofQWNormalisedPolygon::calculateQ(const double efixed, int emode,
                                          const double deltaE,
                                          const double twoTheta,
                                          const double azimuthal) const {
  double ki = 0.0;
  double kf = 0.0;
  if (emode == 1) {
    ki = std::sqrt(efixed * SofQW::energyToK());
    kf = std::sqrt((efixed - deltaE) * SofQW::energyToK());
  } else if (emode == 2) {
    ki = std::sqrt((deltaE + efixed) * SofQW::energyToK());
    kf = std::sqrt(efixed * SofQW::energyToK());
  }
  const double Qx = ki - kf * std::cos(twoTheta);
  const double Qy = -kf * std::sin(twoTheta) * std::cos(azimuthal);
  const double Qz = -kf * std::sin(twoTheta) * std::sin(azimuthal);
  return std::sqrt(Qx * Qx + Qy * Qy + Qz * Qz);
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
    const API::MatrixWorkspace_const_sptr &workspace) {
  const size_t nhist = workspace->getNumberHistograms();
  this->m_theta = std::vector<double>(nhist);
  this->m_thetaWidths = std::vector<double>(nhist);
  // Force phi widths to zero
  this->m_phi = std::vector<double>(nhist, 0.0);
  this->m_phiWidths = std::vector<double>(nhist, 0.0);

  auto inst = workspace->getInstrument();
  const PointingAlong upDir = inst->getReferenceFrame()->pointingUp();

  const auto &spectrumInfo = workspace->spectrumInfo();

  for (size_t i = 0; i < nhist; ++i) // signed for OpenMP
  {
    m_progress->report("Calculating detector angles");

    this->m_theta[i] = -1.0; // Indicates a detector to skip
    this->m_thetaWidths[i] = -1.0;

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

    /**
     * Determine width from shape geometry. A group is assumed to contain
     * detectors with the same shape & r, theta value, i.e. a ring mapped-group
     * The shape is retrieved and rotated to match the rotation of the detector.
     * The angular width is computed using the l2 distance from the sample
     */
    Kernel::V3D pos;
    boost::shared_ptr<const IObject>
        shape; // Defined in its own reference frame with centre at 0,0,0
    Kernel::Quat rot;

    if (spectrumInfo.hasUniqueDetector(i)) {
      pos = det.getPos();
      shape = det.shape();
      rot = det.getRotation();
    } else {
      // assume they all have same shape and same r,theta
      const auto &group = dynamic_cast<const Geometry::DetectorGroup &>(det);
      const auto &firstDet = group.getDetectors();
      pos = firstDet[0]->getPos();
      shape = firstDet[0]->shape();
      rot = firstDet[0]->getRotation();
    }

    double l2(0.0), t(0.0), p(0.0);
    pos.getSpherical(l2, t, p);
    BoundingBox bbox = shape->getBoundingBox();
    auto maxPoint(bbox.maxPoint());
    rot.rotate(maxPoint);
    double boxWidth = maxPoint[upDir];

    m_thetaWidths[i] = std::fabs(2.0 * std::atan(boxWidth / l2));
    if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
      g_log.debug() << "Detector at spectrum ="
                    << workspace->getSpectrum(i).getSpectrumNo()
                    << ", width=" << m_thetaWidths[i] * 180.0 / M_PI
                    << " degrees\n";
    }
  }
}

/**
 * Function that retrieves the two-theta and azimuthal angles from a given
 * detector. It then looks up the nearest neighbours. Using those detectors,
 * it calculates the two-theta and azimuthal angle widths.
 * @param workspace : the workspace containing the needed detector information
 */
void SofQWNormalisedPolygon::initAngularCachesPSD(
    const API::MatrixWorkspace_const_sptr &workspace) {
  const size_t nHistos = workspace->getNumberHistograms();
  g_log.debug() << "Number of Histograms: " << nHistos << '\n';

  bool ignoreMasked = true;
  const int numNeighbours = 4;
  WorkspaceNearestNeighbourInfo neighbourInfo(*workspace, ignoreMasked,
                                              numNeighbours);

  this->m_theta = std::vector<double>(nHistos);
  this->m_thetaWidths = std::vector<double>(nHistos);
  this->m_phi = std::vector<double>(nHistos);
  this->m_phiWidths = std::vector<double>(nHistos);

  const auto &spectrumInfo = workspace->spectrumInfo();

  for (size_t i = 0; i < nHistos; ++i) {
    m_progress->report("Calculating detector angular widths");
    const auto &detector = spectrumInfo.detector(i);
    g_log.debug() << "Current histogram: " << i << '\n';
    specnum_t inSpec = workspace->getSpectrum(i).getSpectrumNo();
    SpectraDistanceMap neighbours = neighbourInfo.getNeighboursExact(inSpec);

    g_log.debug() << "Current ID: " << inSpec << '\n';
    // Convert from spectrum numbers to workspace indices
    double thetaWidth = -DBL_MAX;
    double phiWidth = -DBL_MAX;

    // Find theta and phi widths
    double theta = spectrumInfo.twoTheta(i);
    double phi = detector.getPhi();

    specnum_t deltaPlus1 = inSpec + 1;
    specnum_t deltaMinus1 = inSpec - 1;
    specnum_t deltaPlusT = inSpec + this->m_detNeighbourOffset;
    specnum_t deltaMinusT = inSpec - this->m_detNeighbourOffset;

    for (auto &neighbour : neighbours) {
      specnum_t spec = neighbour.first;
      g_log.debug() << "Neighbor ID: " << spec << '\n';
      if (spec == deltaPlus1 || spec == deltaMinus1 || spec == deltaPlusT ||
          spec == deltaMinusT) {
        const auto &detector_n = spectrumInfo.detector(spec - 1);
        double theta_n = spectrumInfo.twoTheta(spec - 1) * 0.5;
        double phi_n = detector_n.getPhi();

        double dTheta = std::fabs(theta - theta_n);
        double dPhi = std::fabs(phi - phi_n);
        if (dTheta > thetaWidth) {
          thetaWidth = dTheta;
          g_log.information()
              << "Current ThetaWidth: " << thetaWidth * 180 / M_PI << '\n';
        }
        if (dPhi > phiWidth) {
          phiWidth = dPhi;
          g_log.information() << "Current PhiWidth: " << phiWidth * 180 / M_PI
                              << '\n';
        }
      }
    }
    this->m_theta[i] = theta;
    this->m_phi[i] = phi;
    this->m_thetaWidths[i] = thetaWidth;
    this->m_phiWidths[i] = phiWidth;
  }
}

/** Creates the output workspace, setting the axes according to the input
 * binning parameters
 *  @param[in]  inputWorkspace The input workspace
 *  @param[in]  qbinParams The q-bin parameters from the user
 *  @param[out] qAxis The 'vertical' (q) axis defined by the given parameters
 *  @param[out] ebinParams The 'horizontal' (energy) axis parameters (optional)
 *  @return A pointer to the newly-created workspace
 */
RebinnedOutput_sptr SofQWNormalisedPolygon::setUpOutputWorkspace(
    const API::MatrixWorkspace &inputWorkspace,
    const std::vector<double> &qbinParams, std::vector<double> &qAxis,
    const std::vector<double> &ebinParams) {
  using Kernel::VectorHelper::createAxisFromRebinParams;

  HistogramData::BinEdges xAxis(0);
  // Create vector to hold the new X axis values
  if (ebinParams.empty()) {
    xAxis = inputWorkspace.binEdges(0);
  } else {
    static_cast<void>(
        createAxisFromRebinParams(ebinParams, xAxis.mutableRawData()));
  }

  // Create a vector to temporarily hold the vertical ('y') axis and populate
  // that
  const int yLength = static_cast<int>(
      VectorHelper::createAxisFromRebinParams(qbinParams, qAxis));

  // Create output workspace, bin edges are same as in inputWorkspace index 0
  auto outputWorkspace =
      create<RebinnedOutput>(inputWorkspace, yLength - 1, xAxis);

  // Create a binned numeric axis to replace the default vertical one
  Axis *const verticalAxis = new BinEdgeAxis(qAxis);
  outputWorkspace->replaceAxis(1, verticalAxis);

  // Set the axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  outputWorkspace->getAxis(0)->title() = "Energy transfer";

  outputWorkspace->setYUnit("");
  outputWorkspace->setYUnitLabel("Intensity");

  return std::move(outputWorkspace);
}

} // namespace Mantid
} // namespace Algorithms
